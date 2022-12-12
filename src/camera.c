#include "camera.h"
#include "utils/macro_utils.h" // For `CHECK_BITMASK`
#include "utils/map_utils.h" // For `sample_map_point`, and `pos_out_of_overhead_map_bounds`
#include "utils/debug_macro_utils.h" // For `KEY_FLY` (TODO: remove)

////////// Small utility functions

static GLfloat clamp_to_pos_neg_domain(const GLfloat val, const GLfloat limit) {
	return glm_clamp(val, -limit, limit);
}

static void clamp_vec2_to_pos_neg_domain(vec2 val, const GLfloat limit) {
	glm_vec2_clamp(val, -limit, limit);
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
static GLfloat smootherstep(const GLfloat x) {
	return x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f);
}

////////// Angle updating

// This does not include FOV, since FOV depends on a tick's speed, and speed is updated after this function is called
static void update_camera_angles(Angles* const angles, const Event* const event) {
	////////// Vert and hori

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
	const GLfloat
		delta_time = event -> delta_time,
		time_for_full_fov = constants.speeds.xz_max / constants.accel.forward_back;

	const bool accelerating = CHECK_BITMASK(event -> movement_bits, BIT_ACCELERATE);

	GLfloat t = camera -> time_accum_for_full_fov;
	t = accelerating ? fminf(t + delta_time, time_for_full_fov) : fmaxf(t - delta_time, 0.0f);

	const GLfloat fov_percent = smootherstep(t / time_for_full_fov);
	camera -> fov = constants.camera.init_fov + constants.camera.limits.fov_change * fov_percent;
	camera -> time_accum_for_full_fov = t;
}

////////// Collision

typedef struct {
	const bool colliding;
	const byte base_height;
} CollisionInfo;

static CollisionInfo get_pos_collision_info(
	const GLfloat foot_height, const GLfloat* const xz,
	const byte* const heightmap, const byte map_width, const byte map_height) {

	const GLfloat x = xz[0], z = xz[1];

	if (pos_out_of_overhead_map_bounds(x, z, map_width, map_height))
		return (CollisionInfo) {true, 0}; // No valid colliding height if out of bounds

	const byte base_height = sample_map_point(heightmap, (byte) x, (byte) z, map_width);
	const bool colliding = (foot_height - base_height) < -(GLfloat) GLM_FLT_EPSILON;

	return (CollisionInfo) {colliding, base_height};
}

static CollisionInfo get_aabb_collision_info(const GLfloat foot_height,
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
		const CollisionInfo collision_info = get_pos_collision_info(
			foot_height, aabb_corners[i], heightmap, map_width, map_height
		);

		if (collision_info.colliding) return collision_info;
	}

	return (CollisionInfo) {false, sample_map_point(heightmap, (byte) pos_x, (byte) pos_z, map_width)};
}

//////////

static void update_pos(Camera* const camera,
	const byte* const heightmap, const byte map_size[2],
	const vec2 dir_xz, const vec3 pos_before, const Event* const event) {

	////////// Declaring a lot of shared vars

	GLfloat // A float, vec2, and vec3
		*const last_tick_wall_alignment_percent_ref = &camera -> last_tick_wall_alignment_percent,
		*const velocity_xz_view_space = camera -> velocity_xz_view_space, *const pos = camera -> pos;

	const byte movement_bits = event -> movement_bits;
	const GLfloat delta_time = event -> delta_time;

	GLfloat accel_forward_back_per_sec = constants.accel.forward_back;
	if (CHECK_BITMASK(movement_bits, BIT_ACCELERATE)) accel_forward_back_per_sec += constants.accel.additional_forward_back;

	const GLfloat // The `* delta_time` exprs get a per-tick version of each multiplicand
		accel_forward_back_per_tick = accel_forward_back_per_sec * delta_time,
		accel_strafe_per_tick = constants.accel.strafe * delta_time,
		max_speed_xz_per_tick = constants.speeds.xz_max * delta_time;

	////////// Making a view-space velocity per tick

	const GLfloat last_tick_wall_alignment_percent = *last_tick_wall_alignment_percent_ref;

	const bool
		moving_forward = CHECK_BITMASK(movement_bits, BIT_MOVE_FORWARD),
		moving_backward = CHECK_BITMASK(movement_bits, BIT_MOVE_BACKWARD),
		moving_left = CHECK_BITMASK(movement_bits, BIT_STRAFE_LEFT),
		moving_right = CHECK_BITMASK(movement_bits, BIT_STRAFE_RIGHT);

	vec2 accel_per_tick = {accel_forward_back_per_tick, accel_strafe_per_tick};
	glm_vec2_scale(accel_per_tick, delta_time, accel_per_tick);

	vec2 vvs_per_tick; // Note: `vvs` = velocity view-space (just for xz)
	glm_vec2_scale(velocity_xz_view_space, delta_time, vvs_per_tick);
	glm_vec2_add(vvs_per_tick, (vec2) {accel_per_tick[0] * moving_forward, accel_per_tick[1] * moving_left}, vvs_per_tick);
	glm_vec2_sub(vvs_per_tick, (vec2) {accel_per_tick[0] * moving_backward, accel_per_tick[1] * moving_right}, vvs_per_tick);

	////////// Applying floor and wall friction to that

	const GLfloat wall_friction = constants.camera.frictions.wall * last_tick_wall_alignment_percent;;

	const GLfloat
		floor_friction_percentage = get_percent_kept_from(constants.camera.frictions.floor, delta_time),
		wall_friction_percentage = get_percent_kept_from(wall_friction, delta_time);

	if (moving_forward == moving_backward) vvs_per_tick[0] *= floor_friction_percentage;
	if (moving_left == moving_right) vvs_per_tick[1] *= floor_friction_percentage;

	glm_vec2_scale(vvs_per_tick, wall_friction_percentage, vvs_per_tick);

	// Clamping it, and then scaling it to be in terms of seconds
	clamp_vec2_to_pos_neg_domain(vvs_per_tick, max_speed_xz_per_tick);
	glm_vec2_scale(vvs_per_tick, 1.0f / delta_time, velocity_xz_view_space);

	////////// X and Z movement + collision detection

	const GLfloat pace = camera -> pace;
	GLfloat foot_height = pos[1] - constants.camera.eye_height - pace;

	const vec2 pos_xz_next = {
		pos[0] + vvs_per_tick[0] * dir_xz[0] + vvs_per_tick[1] * dir_xz[1],
		pos[2] + vvs_per_tick[0] * dir_xz[1] - vvs_per_tick[1] * dir_xz[0]
	};

	if (!get_aabb_collision_info(foot_height, (vec2) {pos_xz_next[0], pos[2]}, heightmap, map_size).colliding)
		pos[0] = pos_xz_next[0];

	if (!get_aabb_collision_info(foot_height, (vec2) {pos[0], pos_xz_next[1]}, heightmap, map_size).colliding)
		pos[2] = pos_xz_next[1];

	////////// Setting the last tick's wall alignment percent

	const vec2 pos_xz_before = {pos_before[0], pos_before[2]};

	/* Note that these movement dirs are different from `dir_xz`, since
	that's just a direction, but these are directions of movement */

	vec2 movement_dir_xz;
	glm_vec2_sub((vec2) {pos[0], pos[2]}, (GLfloat*) pos_xz_before, movement_dir_xz);
	glm_vec2_normalize(movement_dir_xz);

	vec2 aspired_movement_dir_xz;
	glm_vec2_sub((GLfloat*) pos_xz_next, (GLfloat*) pos_xz_before, aspired_movement_dir_xz);
	glm_vec2_normalize(aspired_movement_dir_xz);

	*last_tick_wall_alignment_percent_ref = fabsf(glm_vec2_cross(movement_dir_xz, aspired_movement_dir_xz));

	////////// Updating the jump/fall velocity, and updating the foot height

	GLfloat speed_jump_per_sec = camera -> jump_fall_velocity;

	if (speed_jump_per_sec == 0.0f && CHECK_BITMASK(movement_bits, BIT_JUMP)) speed_jump_per_sec = constants.speeds.jump;
	else speed_jump_per_sec -= constants.accel.g * delta_time;

	/* Note: the foot height is updated before getting the base height. This is because then,
	the foot height will be partially pressed into the ground, so that a valid AABB collision
	can be found, which leads to an accurate base height being found. */
	foot_height += speed_jump_per_sec * delta_time;

	////////// Setting the new y position and speed

	const byte base_height = get_aabb_collision_info(foot_height,
		(vec2) {pos[0], pos[2]}, heightmap, map_size).base_height;

	const bool continuing_jump_or_fall = foot_height > base_height;


	if (continuing_jump_or_fall)
		pos[1] = foot_height + pace;
	else {
		speed_jump_per_sec = 0.0f;
		pos[1] = base_height;
	}

	pos[1] += constants.camera.eye_height;
	camera -> landed_jump_in_this_tick = !(continuing_jump_or_fall || camera -> jump_fall_velocity == 0.0f);
	camera -> jump_fall_velocity = speed_jump_per_sec;
}

////////// Pace

/* This function models how a player's pace should behave given a time input. At time = 0, the pace is 0.
The function output is always above 0. Period = width of one up-down pulsation, and amplitude = max height. */
static GLfloat make_pace_function(const GLfloat x, const GLfloat period, const GLfloat amplitude) {
	return 0.5f * amplitude * (sinf(x * (TWO_PI / period) + THREE_HALVES_PI) + 1.0f);
}

static void update_pace(Camera* const camera, const GLfloat delta_time) {
	const GLfloat combined_speed_xz_amount = fminf(constants.speeds.xz_max,
		glm_vec2_norm(camera -> velocity_xz_view_space));

	camera -> speed_xz_percent = combined_speed_xz_amount / constants.speeds.xz_max;

	if (camera -> jump_fall_velocity == 0.0f) {
		camera -> pace = make_pace_function(
			camera -> time_since_jump, constants.camera.pace.period,
			constants.camera.pace.max_amplitude * camera -> speed_xz_percent);

		camera -> pos[1] += camera -> pace;
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
	const byte* const heightmap, const byte map_size[2], const vec2 dir_xz) {

	const GLfloat delta_time = event -> delta_time;
	GLfloat* const pos = camera -> pos;

	if (event -> keys[KEY_FLY]) { // Forward, backward, left, right
		const byte movement_bits = event -> movement_bits;

		const GLfloat
			velocity = constants.speeds.xz_max * delta_time,
			*const dir = camera -> dir, *const right = camera -> right;

		#define MOVE(mask, direction, sign)\
			if CHECK_BITMASK(movement_bits, BIT_##mask)\
				glm_vec3_muladds((GLfloat*) direction, sign velocity, pos)

		MOVE(MOVE_FORWARD, dir, );
		MOVE(MOVE_BACKWARD, dir, -);
		MOVE(STRAFE_LEFT, right, -);
		MOVE(STRAFE_RIGHT, right, );

		#undef MOVE
	}
	else {
		vec3 pos_before;
		glm_vec3_copy(pos, pos_before);

		update_pos(camera, heightmap, map_size, dir_xz, pos_before, event);
		update_pace(camera, delta_time);

		GLfloat* const velocity_world_space = camera -> velocity_world_space;
		glm_vec3_sub(pos, pos_before, velocity_world_space);
		glm_vec3_scale(velocity_world_space, 1.0f / delta_time, velocity_world_space);
	}
}

static void update_camera_matrices(Camera* const camera, const GLfloat aspect_ratio) {
	const GLfloat
		*const pos = camera -> pos, *const dir = camera -> dir,
		*const right = camera -> right, *const up = camera -> up;

	vec4* const view = camera -> view;

	#define D(vector, sign) sign glm_vec3_dot((GLfloat*) pos, (GLfloat*) vector)

	// Constructing the view matrix manually because I already have all of the vectors needed for it
	glm_mat4_copy((mat4) {
		{right[0], up[0], -dir[0], 0.0f},
		{right[1], up[1], -dir[1], 0.0f},
		{right[2], up[2], -dir[2], 0.0f},
		{D(right, -), D(up, -), D(dir, ), 1.0f}
	}, view);

	#undef D

	mat4 projection;
	glm_perspective(camera -> fov, aspect_ratio, constants.camera.near_clip_dist, camera -> far_clip_dist, projection);
	glm_mul(projection, view, camera -> view_projection);
}

void update_camera(Camera* const camera, const Event* const event, const byte* const heightmap, const byte map_size[2]) {
	////////// Updating the camera angles

	Angles* const angles = &camera -> angles;
	update_camera_angles(angles, event);

	////////// Defining vectors

	vec2 dir_xz;
	GLfloat *const dir = camera -> dir, *const right = camera -> right, *const up = camera -> up;

	////////// Updating directions, velocity, pos, fov, camera matrices, and frustum planes

	get_camera_directions(angles, dir_xz, dir, camera -> right_xz, right, up);
	update_camera_pos(camera, event, heightmap, map_size, dir_xz);
	update_fov(camera, event);

	update_camera_matrices(camera, event -> aspect_ratio);
	glm_frustum_planes(camera -> view_projection, camera -> frustum_planes);
}

Camera init_camera(const CameraConfig* const config, const GLfloat far_clip_dist) {
	const GLfloat* const init_pos = config -> init_pos;

	return (Camera) {
		.angles = config -> angles, .far_clip_dist = far_clip_dist,
		.pos = {init_pos[0], init_pos[1], init_pos[2]}
	};
}

////////// Sound functions

bool jump_up_sound_activator(const void* const data) {
	return ((Camera*) data) -> jump_fall_velocity == constants.speeds.jump;
}

void jump_up_sound_updater(const void* const data, const ALuint al_source) {
	const Camera* const camera = data;

	/* The jump-up sound has a grunt from the mouth, and a stomp from the feet.
	In order to account for both sounds, I'm averaging the eye and foot pos
	to get a camera center pos that is just between both sound sources.

	TODO: average the pos, dirs, and velocity (they all have different ones). */

	vec3 sound_pos;
	glm_vec3_copy((GLfloat*) camera -> pos, sound_pos);
	sound_pos[1] -= constants.camera.eye_height * 0.5f + camera -> pace;

	alSourcefv(al_source, AL_POSITION, sound_pos);
	alSourcefv(al_source, AL_DIRECTION, camera -> dir);
	alSourcefv(al_source, AL_VELOCITY, camera -> velocity_world_space);
}

bool jump_land_sound_activator(const void* const data) {
	return ((Camera*) data) -> landed_jump_in_this_tick;
}

// TODO: make this sound louder for higher heights (scale the volume percent by a max loudness height)
void jump_land_sound_updater(const void* const data, const ALuint al_source) {
	const Camera* const camera = data;

	vec3 sound_pos;
	glm_vec3_copy((GLfloat*) camera -> pos, sound_pos);
	sound_pos[1] -= constants.camera.eye_height + camera -> pace;

	alSourcefv(al_source, AL_POSITION, sound_pos);
	alSourcefv(al_source, AL_DIRECTION, camera -> dir); // TODO: what should this direction be?
	alSourcefv(al_source, AL_VELOCITY, camera -> velocity_world_space);
}
