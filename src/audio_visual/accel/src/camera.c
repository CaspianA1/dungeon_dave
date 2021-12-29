#ifndef CAMERA_C
#define CAMERA_C

#include "headers/camera.h"
#include "headers/constants.h"
#include "data/maps.c" // TODO: remove

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
	camera -> last_time = SDL_GetTicks() / 1000.0f;
	camera -> bob_input = 0.0f;
	memcpy(camera -> pos, init_pos, sizeof(vec3));
}

static void update_camera_angles(Camera* const camera, const Event* const event, const GLfloat delta_time) {
	const GLfloat look_speed = constants.speeds.look * delta_time;

	GLfloat vert_angle = camera -> angles.vert + look_speed * -event -> mouse_dy;
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

	camera -> angles.hori += look_speed * -event -> mouse_dx;
	camera -> angles.vert = vert_angle;
	camera -> angles.tilt = tilt;
}

static void update_pos_via_physics(const Event* const event,
	PhysicsObject* const physics_obj, const vec2 dir_xz, vec3 pos, const GLfloat last_bob_delta) {

	// Delta time not used for this since movement will become stutter-y otherwise
	const GLfloat ms_per_tick = 1.0f / constants.fps; // TODO: change?

	/* - Crouch
	- Accelerate
	- Clipping before hitting walls head-on
	- Tilt when turning
	- Make speed when pushing against wall depend on surface normal */

	byte* const heightmap = physics_obj -> heightmap;

	const byte
		map_width = physics_obj -> map_size[0], map_height = physics_obj -> map_size[1],
		moving_forward = event -> movement_bits & 1, moving_backward = event -> movement_bits & 2,
		moving_left = event -> movement_bits & 4, moving_right = event -> movement_bits & 8;

	GLfloat
		speed_forward_back = physics_obj -> speeds[0],
		speed_jump = physics_obj -> speeds[1],
		speed_strafe = physics_obj -> speeds[2];

	const GLfloat
		delta_speed_forward_back = a_forward_back * ms_per_tick,
		delta_speed_strafe = a_strafe * ms_per_tick,
		max_speed_xz = xz_v_max * ms_per_tick;

	// TODO: genericize this part
	if (moving_forward && ((speed_forward_back += delta_speed_forward_back) > max_speed_xz)) speed_forward_back = max_speed_xz;
	if (moving_backward && ((speed_forward_back -= delta_speed_forward_back) < -max_speed_xz)) speed_forward_back = -max_speed_xz;
	if (!moving_forward && !moving_backward) speed_forward_back *= speed_decel;

	if (moving_left && ((speed_strafe += delta_speed_strafe) > max_speed_xz)) speed_strafe = max_speed_xz;
	if (moving_right && ((speed_strafe -= delta_speed_strafe) < -max_speed_xz)) speed_strafe = -max_speed_xz;
	if (!moving_left && !moving_right) speed_strafe *= speed_decel;

	////////// X and Z collision detection + setting new positions

	GLfloat foot_height = pos[1] - constants.camera.eye_height - last_bob_delta;

	const GLfloat
		new_x = pos[0] + speed_forward_back * dir_xz[0] - speed_strafe * -dir_xz[1],
		new_z = pos[2] + speed_forward_back * dir_xz[1] - speed_strafe * dir_xz[0];

	if (new_x >= 0.0f && new_x <= map_width - 1.0f) {
		if (*map_point(heightmap, new_x, pos[2], map_width) <= foot_height) pos[0] = new_x;
		// else entity.speed_xz /= 2.0f;
	}

	if (new_z >= 0.0f && new_z <= map_height - 1.0f) {
		if (*map_point(heightmap, pos[0], new_z, map_width) <= foot_height) pos[2] = new_z;
		// else entity.speed_xz /= 2.0f;
	}

	////////// Y collision detection + setting new positions

	if (speed_jump == 0.0f && keys[constants.movement_keys.jump]) speed_jump = yv;
	else speed_jump -= g * ms_per_tick;

	const GLfloat new_y = foot_height + speed_jump * ms_per_tick; // Since g works w acceleration
	const byte height = *map_point(heightmap, pos[0], pos[2], map_width);

	if (new_y > height) foot_height = new_y;
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
	GLfloat bob_delta = 0.0f;
	if (speeds[1] == 0.0f) {
		const GLfloat
			speed_forward_back = fabsf(speeds[0]),
			speed_strafe = fabsf(speeds[2]);

		const GLfloat largest_speed_xz = (speed_forward_back > speed_strafe) ? speed_forward_back : speed_strafe;
		const GLfloat speed_xz_percent = (largest_speed_xz * constants.fps) / xz_v_max;

		// Found through messing around with desmos. With this, y will never go above the eye height.
		bob_delta = speed_xz_percent
			* sinf(3.75f * PI * (camera -> bob_input - 0.1333333333f))
			* 0.1f + 0.1f * speed_xz_percent;

		// pos[1] += bob_delta;
		*pos_y += bob_delta;
		camera -> bob_input += delta_time;
	}
	else camera -> bob_input = 0.0f;

	camera -> last_bob_delta = bob_delta;
}

void update_camera(Camera* const camera, const Event event, PhysicsObject* const physics_obj) {
	const GLfloat curr_time = SDL_GetTicks() / 1000.0f;
	const GLfloat delta_time = curr_time - camera -> last_time;
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
		const GLfloat move_speed = constants.speeds.move * delta_time;
		if (event.movement_bits & 1) glm_vec3_muladds(dir, move_speed, pos);
		if (event.movement_bits & 2) glm_vec3_muladds(dir, -move_speed, pos);
		if (event.movement_bits & 4) glm_vec3_muladds(right, -move_speed, pos);
		if (event.movement_bits & 8) glm_vec3_muladds(right, move_speed, pos);
	}
	else {
		update_pos_via_physics(&event, physics_obj, (vec2) {sin_hori, cos_hori}, pos, camera -> last_bob_delta);
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
