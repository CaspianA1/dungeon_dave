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
	memcpy(camera -> pos, init_pos, sizeof(vec3));
}

// Perhaps pass in a phys obj that can be null

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
	PhysicsObject* const physics_obj, vec3 pos, vec3 dir, const GLfloat delta_time) {

	/* - Manage height properly
	- Strafing
	- Clipping before hitting walls head-on
	- Tilt when turning (make a fn rotate_camera_as_needed for that)
	- Make speed when pushing against wall depend on surface normal
	- Integrate physics system with camera system */

	byte* const heightmap = (byte*) palace_heightmap;
	const byte map_width = palace_width, map_height = palace_height;
	const byte moving_forward = event -> movement_bits & 1, moving_backward = event -> movement_bits & 2;

	GLfloat speed_xz = physics_obj -> speed_xz, speed_y = physics_obj -> speed_y;
	const GLfloat delta_speed = a_xz * delta_time, max_speed_xz = xz_v_max * delta_time;

	if (moving_forward && ((speed_xz += delta_speed) > max_speed_xz)) speed_xz = max_speed_xz;
	if (moving_backward && ((speed_xz -= delta_speed) < -max_speed_xz)) speed_xz = -max_speed_xz;

	if (!moving_forward && !moving_backward) speed_xz *= speed_decel;

	////////// X and Z collision detection + setting new positions

	const GLfloat new_x = pos[0] + speed_xz * dir[0];
	byte height;

	if (new_x >= 0.0f && new_x <= map_width - 1.0f) {
		height = *map_point(heightmap, new_x, pos[2], map_width);
		if (height <= pos[1]) pos[0] = new_x;
		// else entity.speed_xz /= 2.0f;
	}

	const GLfloat new_z = pos[2] + speed_xz * dir[2];
	if (new_z >= 0.0f && new_z <= map_height - 1.0f) {
		height = *map_point(heightmap, pos[0], new_z, map_width);
		if (height <= pos[1]) pos[2] = new_z;
		// else entity.speed_xz /= 2.0f;
	}

	////////// Y collision detection + setting new positions

	if (speed_y == 0.0f && keys[SDL_SCANCODE_SPACE]) speed_y = yv;
	else speed_y -= g * delta_time;

	const GLfloat new_y = pos[1] + speed_y * delta_time; // Since g works w acceleration
	height = *map_point(heightmap, pos[0], pos[2], map_width);

	if (new_y > height) pos[1] = new_y;
	else {
		speed_y = 0.0f;
		pos[1] = height;
	}

	physics_obj -> speed_xz = speed_xz;
	physics_obj -> speed_y = speed_y;
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

	if (physics_obj == NULL) {
		// Forward, backward, left, right
		const GLfloat move_speed = constants.speeds.move * delta_time;
		if (event.movement_bits & 1) glm_vec3_muladds(dir, move_speed, pos);
		if (event.movement_bits & 2) glm_vec3_muladds(dir, -move_speed, pos);
		if (event.movement_bits & 4) glm_vec3_muladds(right, -move_speed, pos);
		if (event.movement_bits & 8) glm_vec3_muladds(right, move_speed, pos);
	}
	else update_pos_via_physics(&event, physics_obj, pos, dir, delta_time);

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
