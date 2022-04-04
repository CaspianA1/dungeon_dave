#ifndef CAMERA_C
#define CAMERA_C

#include "headers/camera.h"
#include "headers/constants.h"

VoxelPhysicsContext init_physics_context(const byte* const heightmap, const byte map_size[2]) {
	const byte map_width = map_size[0], map_height = map_size[1];

	/* The far clip distance, ideally, would be equal to the diameter of
	the convex hull of all points in the heightmap. If I had more time,
	I would implement that, but a simple method that works reasonably well is this:

	- First, find the smallest and tallest points in the map.
	- Then, the far clip distance equals the length of
		the <map_width, map_height, tallest_point - smallest_point> vector. */

	return (VoxelPhysicsContext) {
		(byte*) heightmap, {map_width, map_height},
		0.0f, // TODO: compute this
		{0.0f, 0.0f, 0.0f}
	};
}

void init_camera(Camera* const camera, const vec3 init_pos) {
	camera -> last_time = 0.0f;
	memcpy(&camera -> angles, &constants.camera.init, sizeof(constants.camera.init));
	camera -> pace = 0.0f;
	camera -> speed_xz_percent = 0.0f;
	camera -> time_since_jump = 0.0f;
	camera -> time_accum_for_full_fov = 0.0f;
	glm_vec3_copy((GLfloat*) init_pos, camera -> pos);
}

static GLfloat limit_to_pos_neg_domain(const GLfloat val, const GLfloat limit) {
	if (val > limit) return limit;
	else if (val < -limit) return -limit;
	else return val;
}

// This is framerate-independent.
static GLfloat get_percent_kept_from(const GLfloat magnitude, const GLfloat delta_time) {
	const GLfloat percent_lost = delta_time * magnitude;
	return 1.0f - ((percent_lost < 1.0f) ? percent_lost : 1.0f);
}

// This does not include FOV, since FOV depends on a tick's speed, and speed is updated after this function is called
static void update_camera_angles(Camera* const camera, const Event* const event, const GLfloat delta_time) {
	const int *const mouse_movement = event -> mouse_movement, *const screen_size = event -> screen_size;

	const GLfloat delta_vert = (GLfloat) -mouse_movement[1] / screen_size[1] * constants.speeds.look_vert;
	camera -> angles.vert = limit_to_pos_neg_domain(camera -> angles.vert + delta_vert, constants.camera.lims.vert);

	const GLfloat delta_turn = (GLfloat) -mouse_movement[0] / screen_size[0] * constants.speeds.look_hori;
	camera -> angles.hori += delta_turn;

	const GLfloat tilt = (camera -> angles.tilt + delta_turn * delta_turn)
		* get_percent_kept_from(constants.camera.tilt_correction_rate, delta_time);

	camera -> angles.tilt = limit_to_pos_neg_domain(tilt, constants.camera.lims.tilt);
}

/* Maps a value between 0 and 1 to a smooth output
between 0 and 1, via a cubic Hermite spline. */
static GLfloat smooth_hermite(const GLfloat x) {
	const GLfloat x_squared = x * x;
	return 3.0f * x_squared - 2.0f * x_squared * x;
}

static void update_fov(Camera* const camera, const byte movement_bits, const GLfloat delta_time) {
	/* The time to reach the full FOV equals the time to reach the max X speed.
	Since `v = v0 + at`, and `v0 = 0`, `v = at`, and `t = v / a`. The division
	by the FPS converts the time from being in terms of ticks to seconds. */
	const GLfloat time_for_full_fov = constants.speeds.xz_max / constants.accel.forward_back / constants.fps;
	GLfloat t = camera -> time_accum_for_full_fov;

	if (movement_bits & BIT_ACCELERATE) {
		if ((t += delta_time) > time_for_full_fov) t = time_for_full_fov;
	}
	else if ((t -= delta_time) < 0.0f) t = 0.0f;

	const GLfloat fov_percent = smooth_hermite(t / time_for_full_fov);
	camera -> angles.fov = constants.camera.init.fov + constants.camera.lims.fov * fov_percent;
	camera -> time_accum_for_full_fov = t;
}

static GLfloat apply_velocity_in_xz_direction(const GLfloat curr_v,
	const GLfloat curr_a, const GLfloat delta_time, const GLfloat max_v,
	const bool moving_in_dir, const bool moving_in_opposite_dir) {

	GLfloat v = curr_v + curr_a * moving_in_dir - curr_a * moving_in_opposite_dir;

	// If 0 or 2 directions are being moved in; `^` maps to 1 if only 1 input is true
	if (!(moving_in_dir ^ moving_in_opposite_dir)) v *= get_percent_kept_from(constants.camera.friction, delta_time);
	return limit_to_pos_neg_domain(v, max_v);
}

// Note: `x` and `y` are top-down here.
static bool tile_exists_at_pos(const GLfloat x, const GLfloat y, const GLfloat foot_height,
	const byte* const heightmap, const byte map_width, const byte map_height) {

	if (x < 0.0f || y < 0.0f || x >= map_width || y >= map_height) return 1;

	const byte floor_height = *map_point((byte*) heightmap, (byte) x, (byte) y, map_width);
	return (foot_height - floor_height) < -constants.almost_zero;
}

static bool pos_collides_with_heightmap(const GLfloat foot_height,
	const vec2 pos_xz, const byte* const heightmap, const byte map_size[2]) {

	const byte map_width = map_size[0], map_height = map_size[1];
	const GLfloat half_border = constants.camera.aabb_collision_box_size * 0.5f;

	return tile_exists_at_pos(pos_xz[0] - half_border, pos_xz[1] - half_border, foot_height, heightmap, map_width, map_height)
		|| tile_exists_at_pos(pos_xz[0] + half_border, pos_xz[1] + half_border, foot_height, heightmap, map_width, map_height)
		|| tile_exists_at_pos(pos_xz[0] - half_border, pos_xz[1] + half_border, foot_height, heightmap, map_width, map_height)
		|| tile_exists_at_pos(pos_xz[0] + half_border, pos_xz[1] - half_border, foot_height, heightmap, map_width, map_height);
}

static void update_pos_via_physics(const byte movement_bits,
	VoxelPhysicsContext* const physics_context, const vec2 dir_xz, vec3 pos,
	const GLfloat pace, const GLfloat delta_time) {

	////////// Declaring a lot of shared vars

	GLfloat accel_forward_back_per_sec = constants.accel.forward_back;

	if (movement_bits & BIT_ACCELERATE)
		accel_forward_back_per_sec += constants.accel.additional_forward_back;

	const GLfloat // The `* delta_time` exprs get a per-tick version of each multiplicand
		accel_forward_back = accel_forward_back_per_sec * delta_time,
		accel_strafe = constants.accel.strafe * delta_time,
		max_speed_xz = constants.speeds.xz_max * delta_time;

	GLfloat
		velocity_forward_back = physics_context -> velocities[0] * delta_time,
		velocity_strafe = physics_context -> velocities[2] * delta_time,
		foot_height = pos[1] - constants.camera.eye_height - pace;

	const byte
		*const map_size = physics_context -> map_size,
		*const heightmap = physics_context -> heightmap;

	////////// Updating velocity

	velocity_forward_back = apply_velocity_in_xz_direction(velocity_forward_back, accel_forward_back,
		delta_time, max_speed_xz, !!(movement_bits & BIT_MOVE_FORWARD), !!(movement_bits & BIT_MOVE_BACKWARD));

	velocity_strafe = apply_velocity_in_xz_direction(velocity_strafe, accel_strafe,
		delta_time, max_speed_xz, !!(movement_bits & BIT_STRAFE_LEFT), !!(movement_bits & BIT_STRAFE_RIGHT));

	physics_context -> velocities[0] = velocity_forward_back / delta_time;
	physics_context -> velocities[2] = velocity_strafe / delta_time;

	////////// X and Z collision detection + setting new xz positions

	vec2 pos_xz = {pos[0], pos[2]};

	pos_xz[0] = pos[0] + velocity_forward_back * dir_xz[0] - velocity_strafe * -dir_xz[1];
	if (pos_collides_with_heightmap(foot_height, pos_xz, heightmap, map_size)) pos_xz[0] = pos[0];

	pos_xz[1] = pos[2] + velocity_forward_back * dir_xz[1] - velocity_strafe * dir_xz[0];
	if (pos_collides_with_heightmap(foot_height, pos_xz, heightmap, map_size)) pos_xz[1] = pos[2];

	pos[0] = pos_xz[0];
	pos[2] = pos_xz[1];

	////////// Y collision detection + setting new y position and speed

	GLfloat speed_jump_per_sec = physics_context -> velocities[1];
	if (speed_jump_per_sec == 0.0f && (movement_bits & BIT_JUMP))
		speed_jump_per_sec = constants.speeds.jump;
	else speed_jump_per_sec -= constants.accel.g * delta_time;

	foot_height += speed_jump_per_sec * delta_time;
	const byte base_height = *map_point((byte*) heightmap, (byte) pos[0], (byte) pos[2], map_size[0]);

	if (foot_height > base_height)
		pos[1] = foot_height + pace;
	else {
		speed_jump_per_sec = 0.0f;
		pos[1] = base_height;
	}

	pos[1] += constants.camera.eye_height;
	physics_context -> velocities[1] = speed_jump_per_sec;
}

/* This function models how a player's pace should behave given a time input. At time = 0, the pace is 0.
The function output is always above 0. Period = width of one up-down pulsation, and amplitude = max height. */
static GLfloat make_pace_function(const GLfloat x, const GLfloat period, const GLfloat amplitude) {
	return 0.5f * amplitude * (sinf(x * (TWO_PI / period) + THREE_HALVES_PI) + 1.0f);
}

static void update_pace(Camera* const camera, GLfloat* const pos_y, const vec3 velocities, const GLfloat delta_time) {
	const GLfloat combined_speed_xz_amount = fminf(constants.speeds.xz_max,
		sqrtf(velocities[0] * velocities[0] + velocities[2] * velocities[2]));

	camera -> speed_xz_percent = combined_speed_xz_amount / constants.speeds.xz_max;

	if (velocities[1] == 0.0f) { // Going in the red area results in a lot of slowdown, but only with pace
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

void get_dir_in_2D_and_3D(const GLfloat hori_angle, const GLfloat vert_angle, vec2 dir_xz, vec3 dir) {
	const GLfloat cos_vert = cosf(vert_angle);

	glm_vec2_copy((vec2) {sinf(hori_angle), cosf(hori_angle)}, dir_xz);
	glm_vec3_copy((vec3) {cos_vert * dir_xz[0], sinf(vert_angle), cos_vert * dir_xz[1]}, dir);
}

void update_camera(Camera* const camera, const Event event, VoxelPhysicsContext* const physics_context) {
	const Uint32 curr_time = SDL_GetTicks();
	GLfloat delta_time = (curr_time - camera -> last_time) / 1000.0f;
	camera -> last_time = curr_time;

	update_camera_angles(camera, &event, delta_time);

	////////// Defining vectors

	vec2 dir_xz;
	vec3 dir;

	get_dir_in_2D_and_3D(camera -> angles.hori, camera -> angles.vert, dir_xz, dir); // Outputs dir_xz and dir

	vec3 right = {-dir_xz[1], 0.0f, dir_xz[0]}, up, pos;
	camera -> right_xz[0] = right[0]; // `right_xz` is just like `right`, except that it's not tilted
	camera -> right_xz[1] = right[2];

	glm_vec3_rotate(right, -camera -> angles.tilt, dir); // Outputs a rotated right vector
	glm_vec3_cross(right, dir, up); // Outputs an up vector from the direction and right vectors
	glm_vec3_copy(camera -> pos, pos); // Copying the camera's position vector into a local variable

	////////// Moving according to those vectors

	if (physics_context == NULL || keys[KEY_FLY]) { // Forward, backward, left, right
		const GLfloat speed = constants.speeds.xz_max * delta_time;
		if (event.movement_bits & BIT_MOVE_FORWARD) glm_vec3_muladds(dir, speed, pos);
		if (event.movement_bits & BIT_MOVE_BACKWARD) glm_vec3_muladds(dir, -speed, pos);
		if (event.movement_bits & BIT_STRAFE_LEFT) glm_vec3_muladds(right, -speed, pos);
		if (event.movement_bits & BIT_STRAFE_RIGHT) glm_vec3_muladds(right, speed, pos);
	}
	else {
		update_pos_via_physics(event.movement_bits, physics_context, dir_xz, pos, camera -> pace, delta_time);
		update_pace(camera, pos + 1, physics_context -> velocities, delta_time);
		update_fov(camera, event.movement_bits, delta_time);
	}

	////////// Making some matrices and frustum planes from the new position and the vectors from before

	mat4 view, projection;

	glm_look(pos, dir, up, view);
	glm_perspective(camera -> angles.fov, (GLfloat) event.screen_size[0] / event.screen_size[1],
		constants.camera.clip_dists.near, constants.camera.clip_dists.far, projection);

	// The model matrix is implicit in this, since it equals the identity matrix
	glm_mul(projection, view, camera -> model_view_projection);
	glm_frustum_planes(camera -> model_view_projection, camera -> frustum_planes);

	////////// Copying the local vectors to the camera, and printing important vectors if needed

	glm_vec3_copy(pos, camera -> pos);
	glm_vec3_copy(dir, camera -> dir);
	glm_vec3_copy(right, camera -> right);
	glm_vec3_copy(up, camera -> up);

	if (keys[KEY_PRINT_POSITION]) DEBUG_VEC3(pos);
	if (keys[KEY_PRINT_DIRECTION]) DEBUG_VEC3(dir);
	if (keys[KEY_PRINT_UP]) DEBUG_VEC3(up);
}

#endif
