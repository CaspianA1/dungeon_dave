#ifndef CAMERA_C
#define CAMERA_C

#include "headers/camera.h"
#include "headers/constants.h"

Event get_next_event(void) {
	static GLint viewport_size[4];
	glGetIntegerv(GL_VIEWPORT, viewport_size);

	Event event = {
		.movement_bits =
			keys[constants.movement_keys.forward] |
			(keys[constants.movement_keys.backward] << 1) |
			(keys[constants.movement_keys.left] << 2) |
			(keys[constants.movement_keys.right] << 3) |
			(keys[constants.movement_keys.jump] << 4),

		.screen_size = {viewport_size[2], viewport_size[3]}
	};

	SDL_GetRelativeMouseState(event.mouse_movement, event.mouse_movement + 1);

	return event;
}

void init_camera(Camera* const camera, const vec3 init_pos) {
	memcpy(&camera -> angles, &constants.camera.init, sizeof(constants.camera.init));
	camera -> last_time = SDL_GetPerformanceCounter();
	camera -> time_since_jump = 0.0f;
	memcpy(camera -> pos, init_pos, sizeof(vec3));
}

static GLfloat limit_to_pos_neg_domain(const GLfloat val, const GLfloat limit) {
	if (val > limit) return limit;
	else if (val < -limit) return -limit;
	else return val;
}

static void update_camera_angles(Camera* const camera, const Event* const event) {
	const int *const mouse_movement = event -> mouse_movement, *const screen_size = event -> screen_size;

	const GLfloat delta_vert = (GLfloat) -mouse_movement[1] / screen_size[1] * constants.speeds.look_vert;
	camera -> angles.vert = limit_to_pos_neg_domain(camera -> angles.vert + delta_vert, constants.camera.lims.vert);

	const GLfloat delta_turn = (GLfloat) -mouse_movement[0] / screen_size[0] * constants.speeds.look_hori;
	camera -> angles.hori += delta_turn;

	const GLfloat not_limited_tilt = delta_turn / constants.camera.delta_turn_to_tilt_ratio;
	camera -> angles.tilt = limit_to_pos_neg_domain(not_limited_tilt, constants.camera.lims.tilt);
}

static GLfloat apply_movement_in_xz_direction(const GLfloat curr_v, const GLfloat delta_move,
	const GLfloat max_v, const byte moving_in_dir, const byte moving_in_opposite_dir) {

	GLfloat v = curr_v + delta_move * moving_in_dir - delta_move * moving_in_opposite_dir;

	// If 0 or 2 directions are being moved in; `^` maps to 1 if only 1 input is true
	if (!(moving_in_dir ^ moving_in_opposite_dir)) v *= constants.accel.xz_decel;
	return limit_to_pos_neg_domain(v, max_v);
}

static GLfloat apply_collision_on_xz_axis(
	const byte* const heightmap, const byte map_size[2], const byte varying_axis,
	const vec2 old_pos, const GLfloat foot_height, const GLfloat new_pos_component) {

	if (new_pos_component >= 0.0f && new_pos_component <= map_size[varying_axis]) {
		vec2 new_pos_w_old; // This is top left
		new_pos_w_old[varying_axis] = new_pos_component;
		new_pos_w_old[!varying_axis] = old_pos[!varying_axis];

		const byte height_value = *map_point((byte*) heightmap, new_pos_w_old[0], new_pos_w_old[1], map_size[0]);

		/* `foot_height - height_value > -0.001f` is done instead of `foot_height >= height_value`
		because the foot height may be slightly under the height value (like 2.999999 compared to 3.0,
		due to floating-point precision errors). If `>=` is used, the new position is sometimes not returned. */
		if (foot_height - height_value > -0.001f)
			return new_pos_component;
	}
	return old_pos[varying_axis];
}

static void update_pos_via_physics(const Event* const event,
	PhysicsObject* const physics_obj, const vec2 dir_xz, vec3 pos,
	const GLfloat pace, const GLfloat delta_time) {

	////////// Declaring a lot of shared vars

	const GLfloat // The `* delta_time` exprs get a per-tick version of each multiplicand
		max_speed_xz = constants.speeds.xz_max * delta_time,
		accel_forward_back = constants.accel.forward_back * delta_time,
		accel_strafe = constants.accel.strafe * delta_time;

	GLfloat
		speed_forward_back = physics_obj -> speeds[0] * delta_time,
		speed_strafe = physics_obj -> speeds[2] * delta_time,
		foot_height = pos[1] - constants.camera.eye_height - pace;

	const byte
		movement_bits = event -> movement_bits,
		*const map_size = physics_obj -> map_size,
		*const heightmap = physics_obj -> heightmap;

	////////// Updating speed

	speed_forward_back = apply_movement_in_xz_direction(speed_forward_back,	accel_forward_back,
		max_speed_xz, !!(movement_bits & BIT_MOVE_FORWARD), !!(movement_bits & BIT_MOVE_BACKWARD));

	speed_strafe = apply_movement_in_xz_direction(speed_strafe, accel_strafe,
		max_speed_xz, !!(movement_bits & BIT_STRAFE_LEFT), !!(movement_bits & BIT_STRAFE_RIGHT));

	physics_obj -> speeds[0] = speed_forward_back / delta_time;
	physics_obj -> speeds[2] = speed_strafe / delta_time;

	////////// X and Z collision detection + setting new xz positions

	const vec2 old_pos_xz = {pos[0], pos[2]};

	pos[0] = apply_collision_on_xz_axis(
		heightmap, map_size, 0, old_pos_xz, foot_height,
		pos[0] + speed_forward_back * dir_xz[0] - speed_strafe * -dir_xz[1]);

	pos[2] = apply_collision_on_xz_axis(
		heightmap, map_size, 1, old_pos_xz, foot_height,
		pos[2] + speed_forward_back * dir_xz[1] - speed_strafe * dir_xz[0]);

	////////// Y collision detection + setting new y position and speed

	GLfloat speed_jump_per_sec = physics_obj -> speeds[1];
	if (speed_jump_per_sec == 0.0f && (movement_bits & BIT_JUMP))
		speed_jump_per_sec = constants.speeds.jump;
	else speed_jump_per_sec -= constants.accel.g * delta_time;

	foot_height += speed_jump_per_sec * delta_time;
	const byte base_height = *map_point((byte*) heightmap, pos[0], pos[2], map_size[0]);

	if (foot_height > base_height)
		pos[1] = foot_height + pace;
	else {
		speed_jump_per_sec = 0.0f;
		pos[1] = base_height;
	}

	pos[1] += constants.camera.eye_height;
	physics_obj -> speeds[1] = speed_jump_per_sec;
}

/* This function models how a player's pace should behave given a time input. At time = 0, the pace is 0.
The function output is always above 0. Period = width of one up-down pulsation, and amplitude = max height. */
static GLfloat make_pace_function(const GLfloat x, const GLfloat period, const GLfloat amplitude) {
	return 0.5f * amplitude * (sinf(x * (TWO_PI / period) + THREE_HALVES_PI) + 1.0f);
}

/* Maps a value between 0 and 1 to a smooth output
between 0 and 1, via a cubic Hermite spline. */
static GLfloat smooth_hermite(const GLfloat x) {
	const GLfloat x_squared = x * x;
	return 3.0f * x_squared - 2.0f * x_squared * x;
}

static void update_pace(Camera* const camera, GLfloat* const pos_y, const vec3 speeds, const GLfloat delta_time) {
	if (speeds[1] == 0.0f) { // Going in the red area results in a lot of slowdown, but only with pace
		const GLfloat speed_forward_back = fabsf(speeds[0]), speed_strafe = fabsf(speeds[2]);
		const GLfloat largest_speed_xz = (speed_forward_back > speed_strafe) ? speed_forward_back : speed_strafe;
		const GLfloat speed_xz_percent = smooth_hermite(largest_speed_xz / constants.speeds.xz_max);

		camera -> pace = make_pace_function(
			camera -> time_since_jump, constants.camera.pace.period,
			constants.camera.pace.max_amplitude * speed_xz_percent);

		*pos_y += camera -> pace;
		camera -> time_since_jump += delta_time;
	}
	else {
		camera -> pace = 0.0f;
		camera -> time_since_jump = 0.0f;
	}
}

void update_camera(Camera* const camera, const Event event, PhysicsObject* const physics_obj) {
	static GLfloat one_over_performance_freq;
	static byte first_call = 1;

	if (first_call) {
		one_over_performance_freq = 1.0f / SDL_GetPerformanceFrequency();
		first_call = 0;
	}

	const Uint64 curr_time = SDL_GetPerformanceCounter();
	const GLfloat delta_time = (GLfloat) (curr_time - camera -> last_time) * one_over_performance_freq;
	camera -> last_time = curr_time;

	update_camera_angles(camera, &event);

	const GLfloat
		cos_hori = cosf(camera -> angles.hori), cos_vert = cosf(camera -> angles.vert),
		sin_hori = sinf(camera -> angles.hori), sin_vert = sinf(camera -> angles.vert);

	camera -> right_xz[0] = -cos_hori;
	camera -> right_xz[1] = sin_hori;

	vec3
		dir = {cos_vert * sin_hori, sin_vert, cos_vert * cos_hori},
		right = {camera -> right_xz[0], 0.0f, camera -> right_xz[1]}, pos;

	memcpy(pos, camera -> pos, sizeof(vec3));

	if (physics_obj == NULL || keys[KEY_FLY]) { // Forward, backward, left, right
		const GLfloat speed = constants.speeds.xz_max * delta_time;
		if (event.movement_bits & BIT_MOVE_FORWARD) glm_vec3_muladds(dir, speed, pos);
		if (event.movement_bits & BIT_MOVE_BACKWARD) glm_vec3_muladds(dir, -speed, pos);
		if (event.movement_bits & BIT_STRAFE_LEFT) glm_vec3_muladds(right, -speed, pos);
		if (event.movement_bits & BIT_STRAFE_RIGHT) glm_vec3_muladds(right, speed, pos);
	}
	else {
		update_pos_via_physics(&event, physics_obj, (vec2) {sin_hori, cos_hori}, pos, camera -> pace, delta_time);
		update_pace(camera, pos + 1, physics_obj -> speeds, delta_time);
	}

	memcpy(camera -> pos, pos, sizeof(vec3));

	if (keys[KEY_PRINT_POSITION])
		printf("pos = {%lf, %lf, %lf}\n", (double) pos[0], (double) pos[1], (double) pos[2]);

	vec3 rel_origin, up;
	glm_vec3_rotate(right, -camera -> angles.tilt, dir); // Roll applied after input as to not interfere with the camera movement
	glm_vec3_add(pos, dir, rel_origin);
	glm_vec3_cross(right, dir, up);

	//////////

	mat4 view, projection;
	glm_lookat(pos, rel_origin, up, view);

	glm_perspective(camera -> angles.fov, (GLfloat) event.screen_size[0] / event.screen_size[1],
		constants.camera.clip_dists.near, constants.camera.clip_dists.far, projection);

	glm_mul(projection, view, camera -> view_projection); // For billboard shader
	glm_mul(camera -> view_projection, (mat4) GLM_MAT4_IDENTITY_INIT, camera -> model_view_projection); // For sector shader
}

#endif
