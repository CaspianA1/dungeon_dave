#ifndef CAMERA_C
#define CAMERA_C

#include "headers/camera.h"
#include "headers/constants.h"

Event get_next_event(void) {
	static GLint viewport_size[4];
	glGetIntegerv(GL_VIEWPORT, viewport_size);

	Event e = {
		.movement_bits =
			(keys[constants.movement_keys.tilt_left] << 5) |
			(keys[constants.movement_keys.tilt_right] << 4) |
			(keys[constants.movement_keys.right] << 3) |
			(keys[constants.movement_keys.left] << 2) |
			(keys[constants.movement_keys.backward] << 1) |
			keys[constants.movement_keys.forward],
		.screen_size = {viewport_size[2], viewport_size[3]}
	};

	SDL_GetRelativeMouseState(&e.mouse_dx, &e.mouse_dy);

	return e;
}

void init_camera(Camera* const camera, const vec3 init_pos) {
	memcpy(&camera -> angles, &constants.camera.init, sizeof(constants.camera.init));
	camera -> last_time = SDL_GetPerformanceCounter();
	camera -> time_since_last_jump = 0.0f;
	memcpy(camera -> pos, init_pos, sizeof(vec3));
}

static void update_camera_angles(Camera* const camera, const Event* const event, const GLfloat delta_time) {
	GLfloat vert_angle = camera -> angles.vert + constants.speeds.look * -event -> mouse_dy;
	if (vert_angle > constants.camera.lims.vert) vert_angle = constants.camera.lims.vert;
	else if (vert_angle < -constants.camera.lims.vert) vert_angle = -constants.camera.lims.vert;

	//////////

	GLfloat tilt = camera -> angles.tilt;
	const GLfloat tilt_speed = constants.speeds.tilt * delta_time;

	if ((event -> movement_bits & 32) && ((tilt += tilt_speed) > constants.camera.lims.tilt)) // Left
		tilt = constants.camera.lims.tilt - 0.01f;

	if ((event -> movement_bits & 16) && ((tilt -= tilt_speed) < -constants.camera.lims.tilt)) // Right
		tilt = -constants.camera.lims.tilt + 0.01f;

	//////////

	camera -> angles.hori += constants.speeds.look * -event -> mouse_dx;
	camera -> angles.vert = vert_angle;
	camera -> angles.tilt = tilt;
}

static float apply_movement_in_xz_direction(const GLfloat curr_v, const GLfloat delta_move,
	const GLfloat max_v, const byte moving_in_dir, const byte moving_in_opposite_dir) {

	float v = curr_v;

	v += delta_move * moving_in_dir;
	v -= delta_move * moving_in_opposite_dir;

	if (!moving_in_dir && !moving_in_opposite_dir) v *= constants.accel.xz_decel;

	if (v > max_v) return max_v;
	else if (v < -max_v) return -max_v;
	else return v;
}

static void update_pos_via_physics(const Event* const event,
	PhysicsObject* const physics_obj, const vec2 dir_xz, vec3 pos,
	const GLfloat bob_delta, const GLfloat delta_time) {

	/* - Crouch
	- Accelerate through pressing a key
	- Clipping before hitting walls head-on
	- Tilt when turning */

	byte* const heightmap = physics_obj -> heightmap;
	const byte map_width = physics_obj -> map_size[0], map_height = physics_obj -> map_size[1];

	GLfloat
		speed_forward_back = physics_obj -> speeds[0],
		speed_jump = physics_obj -> speeds[1],
		speed_strafe = physics_obj -> speeds[2];

	const GLfloat max_speed_xz = constants.speeds.xz_max * delta_time;

	speed_forward_back = apply_movement_in_xz_direction(
		speed_forward_back,	constants.accel.forward_back * delta_time, max_speed_xz,
		event -> movement_bits & 1, event -> movement_bits & 2);

	speed_strafe = apply_movement_in_xz_direction(
		speed_strafe, constants.accel.strafe * delta_time, max_speed_xz,
		event -> movement_bits & 4, event -> movement_bits & 8);

	////////// X and Z collision detection + setting new positions

	GLfloat foot_height = pos[1] - constants.camera.eye_height - bob_delta;
	if (foot_height < 0.0f) foot_height = 0.0f;

	const GLfloat
		new_x = pos[0] + speed_forward_back * dir_xz[0] - speed_strafe * -dir_xz[1],
		new_z = pos[2] + speed_forward_back * dir_xz[1] - speed_strafe * dir_xz[0];

	if (new_x >= 0.0f && new_x <= map_width) {
		if (*map_point(heightmap, new_x, pos[2], map_width) <= foot_height) pos[0] = new_x;
		// else entity.speed_xz /= 2.0f;
	}

	if (new_z >= 0.0f && new_z <= map_height) {
		if (*map_point(heightmap, pos[0], new_z, map_width) <= foot_height) pos[2] = new_z;
		// else entity.speed_xz /= 2.0f;
	}

	////////// Y collision detection + setting new positions

	if (speed_jump == 0.0f && keys[constants.movement_keys.jump]) speed_jump = constants.speeds.y_jump;
	speed_jump -= constants.accel.g * delta_time;

	const GLfloat new_y = foot_height + speed_jump * delta_time; // Since g works w acceleration
	const byte height = *map_point(heightmap, pos[0], pos[2], map_width);

	if (new_y > height) foot_height = new_y + bob_delta;
	else {
		speed_jump = 0.0f;
		foot_height = height;
	}

	pos[1] = foot_height + constants.camera.eye_height;
	physics_obj -> speeds[0] = speed_forward_back;
	physics_obj -> speeds[1] = speed_jump;
	physics_obj -> speeds[2] = speed_strafe;
}

static void update_bob(Camera* const camera, GLfloat* const pos_y, vec3 speeds, const GLfloat delta_time) {
	const GLfloat speed_forward_back = fabsf(speeds[0]), speed_strafe = fabsf(speeds[2]);
	const GLfloat largest_speed_xz = (speed_forward_back > speed_strafe) ? speed_forward_back : speed_strafe;
	const GLfloat speed_xz_percent = (largest_speed_xz / delta_time) / constants.speeds.xz_max;

	// Going in the red area results in a lot of slowdown, but only with bob

	if (speeds[1] == 0.0f) {
		const GLfloat x = camera -> time_since_last_jump;
		const GLfloat pace_curve_val = (sinf(4.0f * PI * (x + 0.375f)) + 1.0f) / 16.0f;
		camera -> bob_delta = pace_curve_val * speed_xz_percent;
		*pos_y += camera -> bob_delta;
		camera -> time_since_last_jump += delta_time;
	}
	else camera -> time_since_last_jump = 0.0f;
}

void update_camera(Camera* const camera, const Event event, PhysicsObject* const physics_obj) {
	static byte first_call = 1;
	static GLfloat one_over_performance_frequency;

	if (first_call) {
		one_over_performance_frequency = 1.0f / SDL_GetPerformanceFrequency();
		first_call = 0;
	}

	const Uint64 curr_time = SDL_GetPerformanceCounter();
	const GLfloat delta_time = (GLfloat) (curr_time - camera -> last_time) * one_over_performance_frequency;
	camera -> last_time = curr_time;

	update_camera_angles(camera, &event, delta_time);

	const GLfloat
		cos_hori = cosf(camera -> angles.hori), cos_vert = cosf(camera -> angles.vert),
		sin_hori = sinf(camera -> angles.hori), sin_vert = sinf(camera -> angles.vert);

	camera -> right_xz[0] = -cos_hori;
	camera -> right_xz[1] = sin_hori;

	vec3 dir = {
		cos_vert * sin_hori, sin_vert, cos_vert * cos_hori
	}, right = {camera -> right_xz[0], 0.0f, camera -> right_xz[1]}, pos;

	memcpy(pos, camera -> pos, sizeof(vec3));

	if (physics_obj == NULL) { // Forward, backward, left, right
		const GLfloat speed = constants.speeds.xz_max * delta_time;
		if (event.movement_bits & 1) glm_vec3_muladds(dir, speed, pos);
		if (event.movement_bits & 2) glm_vec3_muladds(dir, -speed, pos);
		if (event.movement_bits & 4) glm_vec3_muladds(right, -speed, pos);
		if (event.movement_bits & 8) glm_vec3_muladds(right, speed, pos);
	}
	else {
		update_pos_via_physics(&event, physics_obj, (vec2) {sin_hori, cos_hori}, pos, camera -> bob_delta, delta_time);
		update_bob(camera, pos + 1, physics_obj -> speeds, delta_time);
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
