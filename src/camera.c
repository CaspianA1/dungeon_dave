#ifndef CAMERA_C
#define CAMERA_C

#include "headers/camera.h"
#include "headers/constants.h"

////////// Small utility functions

static GLfloat clamp_to_pos_neg_domain(const GLfloat val, const GLfloat limit) {
	if (val > limit) return limit;
	else if (val < -limit) return -limit;
	else return val;
}

// If a value is smaller or larger than an edge of the domain, it is wrapped around to the other side.
static GLfloat wrap_around_domain(const GLfloat val, const GLfloat lower, const GLfloat upper) {
	if (val < lower) return upper;
	else if (val > upper) return lower;
	else return val;
}

// This is framerate-independent.
static GLfloat get_percent_kept_from(const GLfloat magnitude, const GLfloat delta_time) {
	const GLfloat percent_lost = delta_time * magnitude;
	return 1.0f - fminf(percent_lost, 1.0f);
}

/* From https://en.wikipedia.org/wiki/Smoothstep. This is
Ken Perlin's improvement of the typical `smoothstep`. */
static GLfloat smooth_hermite(const GLfloat x) {
	return x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f);
}

////////// Angle updating

// This does not include FOV, since FOV depends on a tick's speed, and speed is updated after this function is called
static void update_camera_angles(Angles* const angles, const Event* const event) {
	const GLfloat* const mouse_movement_percent = event -> mouse_movement_percent;

	const GLfloat delta_vert = mouse_movement_percent[1] * constants.speeds.look[1];
	angles -> vert = clamp_to_pos_neg_domain(angles -> vert + delta_vert, constants.camera.limits.vert_max);

	const GLfloat hori_mouse_movement_percent = mouse_movement_percent[0];

	const GLfloat delta_hori = hori_mouse_movement_percent * constants.speeds.look[0];
	angles -> hori = wrap_around_domain(angles -> hori + delta_hori, 0.0f, constants.camera.limits.hori_wrap_around);

	////////// Tilt

	/* Not necessary to account for a sign of 0, since if
	`hori_mouse_movement_percent` is 0, `delta_hori` will be 0 too */
	const GLint tilt_sign = (hori_mouse_movement_percent > 0.0f) ? -1 : 1;

	// Without the turn sign, the camera would only tilt one direction
	const GLfloat tilt = (angles -> tilt + delta_hori * delta_hori * tilt_sign)
		* get_percent_kept_from(constants.camera.tilt_correction_rate, event -> delta_time);

	angles -> tilt = clamp_to_pos_neg_domain(tilt, constants.camera.limits.tilt_max);
}

static void update_fov(Camera* const camera, const Event* const event) {
	/* The time to reach the full FOV equals the time to reach the max X speed.
	Since `v = v0 + at`, and `v0 = 0`, `v = at`, and `t = v / a`. The division
	by the FPS (or the refresh rate) converts the time from being in terms of ticks to seconds. */

	const GLfloat
		delta_time = event -> delta_time,
		time_for_full_fov = constants.speeds.xz_max
			/ constants.accel.forward_back
			/ get_runtime_constant(RefreshRate);

	const bool accelerating = CHECK_BITMASK(event -> movement_bits, BIT_ACCELERATE);

	GLfloat t = camera -> time_accum_for_full_fov;
	t = accelerating ? fminf(t + delta_time, time_for_full_fov) : fmaxf(t - delta_time, 0.0f);

	const GLfloat fov_percent = smooth_hermite(t / time_for_full_fov);
	camera -> angles.fov = constants.camera.init.fov + constants.camera.limits.fov_change * fov_percent;
	camera -> time_accum_for_full_fov = t;
}

////////// Physics + collision

static GLfloat apply_velocity_in_xz_direction(const GLfloat curr_v,
	const GLfloat curr_a, const GLfloat delta_time, const GLfloat max_v,
	const bool moving_in_dir, const bool moving_in_opposite_dir) {

	GLfloat v = curr_v + curr_a * moving_in_dir - curr_a * moving_in_opposite_dir;

	// If 0 or 2 directions are being moved in; `^` maps to 1 if only 1 input is true
	if (!(moving_in_dir ^ moving_in_opposite_dir)) v *= get_percent_kept_from(constants.camera.friction, delta_time);
	return clamp_to_pos_neg_domain(v, max_v);
}

static bool tile_exists_at_pos(const GLfloat x, const GLfloat z, const GLfloat foot_height,
	const byte* const heightmap, const byte map_width, const byte map_height) {

	if (x < 0.0f || z < 0.0f || x >= map_width || z >= map_height) return 1;

	const byte floor_height = sample_map_point(heightmap, (byte) x, (byte) z, map_width);
	return (foot_height - floor_height) < -constants.almost_zero;
}

static bool pos_collides_with_heightmap(const GLfloat foot_height,
	const vec2 pos_xz, const byte* const heightmap, const byte map_size[2]) {

	const byte map_width = map_size[0], map_height = map_size[1];
	const GLfloat half_border = constants.camera.aabb_collision_box_size * 0.5f;

	const GLfloat pos_x = pos_xz[0], pos_z = pos_xz[1];

	// Top left, top right, bottom left, bottom right
	const vec2 aabb_corners[corners_per_quad] = {
		{pos_x - half_border, pos_z - half_border},
		{pos_x + half_border, pos_z - half_border},
		{pos_x - half_border, pos_z + half_border},
		{pos_x + half_border, pos_z + half_border}
	};

	for (byte i = 0; i < corners_per_quad; i++) {
		const GLfloat* const aabb_corner = aabb_corners[i];

		if (tile_exists_at_pos(aabb_corner[0], aabb_corner[1], foot_height,
			heightmap, map_width, map_height)) return true;
	}

	return false;
}

static void update_pos_via_physics(const byte* const heightmap,
	const byte map_size[2], const vec2 dir_xz, vec3 pos,
	vec3 velocities, const GLfloat pace, const Event* const event) {

	const byte movement_bits = event -> movement_bits;
	const GLfloat delta_time = event -> delta_time;

	////////// Declaring a lot of shared vars

	GLfloat accel_forward_back_per_sec = constants.accel.forward_back;

	if (CHECK_BITMASK(movement_bits, BIT_ACCELERATE))
		accel_forward_back_per_sec += constants.accel.additional_forward_back;

	const GLfloat // The `* delta_time` exprs get a per-tick version of each multiplicand
		accel_forward_back = accel_forward_back_per_sec * delta_time,
		accel_strafe = constants.accel.strafe * delta_time,
		max_speed_xz = constants.speeds.xz_max * delta_time;

	GLfloat
		velocity_forward_back = velocities[0] * delta_time,
		velocity_strafe = velocities[2] * delta_time,
		foot_height = pos[1] - constants.camera.eye_height - pace;

	////////// Updating velocity

	velocity_forward_back = apply_velocity_in_xz_direction(velocity_forward_back, accel_forward_back,
		delta_time, max_speed_xz, CHECK_BITMASK(movement_bits, BIT_MOVE_FORWARD),
		CHECK_BITMASK(movement_bits, BIT_MOVE_BACKWARD));

	velocity_strafe = apply_velocity_in_xz_direction(velocity_strafe, accel_strafe,
		delta_time, max_speed_xz, CHECK_BITMASK(movement_bits, BIT_STRAFE_LEFT),
		CHECK_BITMASK(movement_bits, BIT_STRAFE_RIGHT));

	const GLfloat one_over_delta_time = 1.0f / delta_time;
	velocities[0] = velocity_forward_back * one_over_delta_time;
	velocities[2] = velocity_strafe * one_over_delta_time;

	////////// X and Z collision detection + setting new xz positions

	vec2 pos_xz = {pos[0], pos[2]};

	pos_xz[0] = pos[0] + velocity_forward_back * dir_xz[0] - velocity_strafe * -dir_xz[1];
	if (pos_collides_with_heightmap(foot_height, pos_xz, heightmap, map_size)) pos_xz[0] = pos[0];

	pos_xz[1] = pos[2] + velocity_forward_back * dir_xz[1] - velocity_strafe * dir_xz[0];
	if (pos_collides_with_heightmap(foot_height, pos_xz, heightmap, map_size)) pos_xz[1] = pos[2];

	pos[0] = pos_xz[0];
	pos[2] = pos_xz[1];

	////////// Y collision detection + setting new y position and speed

	GLfloat speed_jump_per_sec = velocities[1];
	if (speed_jump_per_sec == 0.0f && CHECK_BITMASK(movement_bits, BIT_JUMP))
		speed_jump_per_sec = constants.speeds.jump;
	else speed_jump_per_sec -= constants.accel.g * delta_time;

	foot_height += speed_jump_per_sec * delta_time;
	const byte base_height = sample_map_point(heightmap, (byte) pos[0], (byte) pos[2], map_size[0]);

	if (foot_height > base_height)
		pos[1] = foot_height + pace;
	else {
		speed_jump_per_sec = 0.0f;
		pos[1] = base_height;
	}

	pos[1] += constants.camera.eye_height;
	velocities[1] = speed_jump_per_sec;
}

////////// Pace

/* This function models how a player's pace should behave given a time input. At time = 0, the pace is 0.
The function output is always above 0. Period = width of one up-down pulsation, and amplitude = max height. */
static GLfloat make_pace_function(const GLfloat x, const GLfloat period, const GLfloat amplitude) {
	return 0.5f * amplitude * (sinf(x * (TWO_PI / period) + THREE_HALVES_PI) + 1.0f);
}

static void update_pace(Camera* const camera, GLfloat* const pos_y, const vec3 velocities, const GLfloat delta_time) {
	const GLfloat combined_speed_xz_amount = fminf(constants.speeds.xz_max,
		glm_vec2_norm((vec2) {velocities[0], velocities[2]}));

	camera -> speed_xz_percent = combined_speed_xz_amount / constants.speeds.xz_max;

	if (velocities[1] == 0.0f) {
		camera -> pace = make_pace_function(
			camera -> time_since_jump, constants.camera.pace.period,
			constants.camera.pace.max_amplitude * camera -> speed_xz_percent);

		*pos_y += camera -> pace;
		camera -> time_since_jump += delta_time;
	}
	else {
		camera -> pace = 0.0f;
		camera -> time_since_jump = 0.0f;
	}
}

////////// Miscellaneous

static void get_camera_directions(const Angles* const angles, vec2 dir_xz, vec3 dir, vec2 right_xz, vec3 right, vec3 up) {
	const GLfloat hori_angle = angles -> hori, vert_angle = angles -> vert;

	const GLfloat
		sin_vert = sinf(vert_angle), cos_vert = cosf(vert_angle),
		sin_hori = sinf(hori_angle), cos_hori = cosf(hori_angle);

	dir_xz[0] = sin_hori;
	dir_xz[1] = cos_hori;

	dir[0] = cos_vert * sin_hori;
	dir[1] = sin_vert;
	dir[2] = cos_vert * cos_hori;

	right_xz[0] = -cos_hori;
	right_xz[1] = sin_hori;

	right[0] = -cos_hori;
	right[1] = 0.0f;
	right[2] = sin_hori;

	glm_vec3_rotate(right, angles -> tilt, dir); // Outputs a rotated right vector
	glm_vec3_cross(right, dir, up); // Outputs an up vector from the direction and right vectors
}

static void update_camera_pos(Camera* const camera, const Event* const event,
	const byte* const heightmap, const byte map_size[2],
	const vec2 dir_xz, const vec3 dir, const vec3 right) {

	const GLfloat delta_time = event -> delta_time;
	GLfloat* const pos = camera -> pos;

	if (event -> keys[KEY_FLY]) { // Forward, backward, left, right
		const byte movement_bits = event -> movement_bits;
		const GLfloat velocity = constants.speeds.xz_max * delta_time;

		if (CHECK_BITMASK(movement_bits, BIT_MOVE_FORWARD)) glm_vec3_muladds((GLfloat*) dir, velocity, pos);
		if (CHECK_BITMASK(movement_bits, BIT_MOVE_BACKWARD)) glm_vec3_muladds((GLfloat*) dir, -velocity, pos);
		if (CHECK_BITMASK(movement_bits, BIT_STRAFE_LEFT)) glm_vec3_muladds((GLfloat*) right, -velocity, pos);
		if (CHECK_BITMASK(movement_bits, BIT_STRAFE_RIGHT)) glm_vec3_muladds((GLfloat*) right, velocity, pos);
	}
	else {
		GLfloat* const velocities = camera -> velocities;
		update_pos_via_physics(heightmap, map_size, dir_xz, pos, velocities, camera -> pace, event);
		update_pace(camera, pos + 1, velocities, delta_time);
		update_fov(camera, event);
	}
}

static void update_camera_matrices(Camera* const camera, const vec3 dir, const vec3 up, const vec3 right) {
	const GLfloat* const pos = camera -> pos;
	vec4* const view = camera -> view;

	#define d(vector, sign) sign glm_dot((GLfloat*) pos, (GLfloat*) vector)

	// Constructing the view matrix manually because I already have all of the vectors needed for it
	glm_mat4_copy((mat4) {
		{right[0], up[0], -dir[0], 0.0f},
		{right[1], up[1], -dir[1], 0.0f},
		{right[2], up[2], -dir[2], 0.0f},
		{d(right, -), d(up, -), d(dir, ), 1.0f}
	}, view);

	#undef d

	mat4 projection;
	glm_perspective(camera -> angles.fov, camera -> aspect_ratio, constants.camera.near_clip_dist, camera -> far_clip_dist, projection);
	glm_mul(projection, view, camera -> view_projection);
}

void update_camera(Camera* const camera, const Event event, const byte* const heightmap, const byte map_size[2]) {
	////////// Updating the camera aspect ratio and angles

	camera -> aspect_ratio = (GLfloat) event.screen_size[0] / event.screen_size[1];

	Angles* const angles = &camera -> angles;
	update_camera_angles(angles, &event);

	////////// Defining vectors

	vec2 dir_xz;
	vec3 right;
	GLfloat* const dir = camera -> dir, *const up = camera -> up;

	get_camera_directions(angles, dir_xz, dir, camera -> right_xz, right, up);
	update_camera_pos(camera, &event, heightmap, map_size, dir_xz, dir, right); // Updates `pos`

	update_camera_matrices(camera, dir, up, right); // Updates `view` and `model_view_projection`
	glm_frustum_planes(camera -> view_projection, camera -> frustum_planes);

	////////// Printing important vectors if needed

	if (event.keys[KEY_PRINT_POSITION]) DEBUG_VEC3(camera -> pos);
	if (event.keys[KEY_PRINT_DIRECTION]) DEBUG_VEC3(dir);
}

Camera init_camera(const vec3 init_pos, const GLfloat far_clip_dist) {
	Camera camera = {.angles = constants.camera.init, .far_clip_dist = far_clip_dist};
	glm_vec3_copy((GLfloat*) init_pos, camera.pos);
	return camera;
}

GLfloat compute_world_far_clip_dist(const byte* const heightmap, const byte map_size[2]) {
	/* The far clip distance, ideally, would be equal to the diameter of
	the convex hull of all points in the heightmap. If I had more time,
	I would implement that, but a simple method that works reasonably well is this:

	- First, find the smallest and tallest points in the map.
	- Then, the far clip distance equals the length of
		the `<map_width, map_height, (tallest_point - smallest_point) + additional_camera_height>` vector.

	To compute the maximum jump height, use the kinematics equation `v^2 = v0^2 + 2aΔy`.
	Given that `v` equals 0, rearrange the equation like this:

	0 = v0^2 + 2aΔy
	-(v0^2) = 2aΔy
	-(v0^2)/2a = Δy

	And since downward acceleration is positive in `constants`, to not get a negative result,
	remove the negative sign of the left term.

	Then, `additional_camera_height` equals `max_jump_height + eye_height`. */

	const byte map_size_x = map_size[0], map_size_z = map_size[1];

	byte min_point_height = constants.max_byte_value, max_point_height = 0;

	for (byte y = 0; y < map_size_z; y++) {
		for (byte x = 0; x < map_size_x; x++) {
			const byte height = sample_map_point(heightmap, x, y, map_size_x);
			if (height < min_point_height) min_point_height = height;
			if (height > max_point_height) max_point_height = height;
		}
	}

	const GLfloat max_jump_height = (constants.speeds.jump * constants.speeds.jump) / (2.0f * constants.accel.g);
	const GLfloat additional_camera_height = max_jump_height + constants.camera.eye_height;

	const GLfloat max_z_difference = (max_point_height - min_point_height) + additional_camera_height;
	return glm_vec3_norm((vec3) {map_size_x, map_size_z, max_z_difference});
}

#endif
