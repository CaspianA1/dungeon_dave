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
	*camera = (Camera) {
		.fov = constants.camera.init.fov,
		.hori_angle = constants.camera.init.hori,
		.vert_angle = constants.camera.init.vert,
		.tilt_angle = constants.camera.init.tilt,
		.last_time = SDL_GetTicks() / 1000.0f,
		.pos = {init_pos[0], init_pos[1], init_pos[2]}
	};
}

void update_camera_from_physics_entity(Camera* const camera, const PhysicsEntity entity) {
	(void) camera;
	(void) entity;
	// TODO: implement this
}

void update_camera(Camera* const camera, const Event event) {
	const GLfloat curr_time = SDL_GetTicks() / 1000.0f;
	const GLfloat delta_time = curr_time - camera -> last_time;
	camera -> last_time = curr_time;

	const GLfloat
		look_speed = constants.speeds.look * delta_time,
		// move_speed = constants.speeds.move * delta_time,
		tilt_speed = constants.speeds.tilt * delta_time;

	camera -> hori_angle += look_speed * -event.mouse_dx;
	camera -> vert_angle += look_speed * -event.mouse_dy;

	if (camera -> vert_angle > constants.camera.lims.vert) camera -> vert_angle = constants.camera.lims.vert;
	else if (camera -> vert_angle < -constants.camera.lims.vert) camera -> vert_angle = -constants.camera.lims.vert;

	const GLfloat
		cos_vert = cosf(camera -> vert_angle),
		hori_angle_minus_half_pi = camera -> hori_angle - HALF_PI;

	camera -> right_xz[0] = sinf(hori_angle_minus_half_pi);
	camera -> right_xz[1] = cosf(hori_angle_minus_half_pi);

	//////////

	GLfloat tilt = camera -> tilt_angle;

	if ((event.movement_bits & 32) && ((tilt += tilt_speed) > constants.camera.lims.tilt)) // Left
		tilt = constants.camera.lims.tilt - 0.01f;

	if ((event.movement_bits & 16) && ((tilt -= tilt_speed) < -constants.camera.lims.tilt)) // Right
		tilt = -constants.camera.lims.tilt + 0.01f;

	camera -> tilt_angle = tilt;

	//////////

	vec3 dir = {
		cos_vert * sinf(camera -> hori_angle), sinf(camera -> vert_angle), cos_vert * cosf(camera -> hori_angle)
	}, right = {camera -> right_xz[0], 0.0f, camera -> right_xz[1]}, pos;

	memcpy(pos, camera -> pos, sizeof(vec3));

	// Forward, backward, left, right
	/* TODO: add back once a separate physics-connected camera fn has been made
	if (event.movement_bits & 1) glm_vec3_muladds(dir, move_speed, pos);
	if (event.movement_bits & 2) glm_vec3_muladds(dir, -move_speed, pos);
	if (event.movement_bits & 4) glm_vec3_muladds(right, -move_speed, pos);
	if (event.movement_bits & 8) glm_vec3_muladds(right, move_speed, pos);
	memcpy(camera -> pos, pos, sizeof(vec3));
	*/

	if (keys[KEY_PRINT_POSITION])
		printf("pos = {%lf, %lf, %lf}\n", (double) pos[0], (double) pos[1], (double) pos[2]);

	vec3 rel_origin, up;
	glm_vec3_rotate(right, -tilt, dir); // Roll applied after input as to not interfere with the camera movement
	glm_vec3_add(pos, dir, rel_origin);
	glm_vec3_cross(right, dir, up);

	//////////

	mat4 view, projection;
	glm_lookat(pos, rel_origin, up, view);

	glm_perspective(camera -> fov, (GLfloat) event.screen_size[0] / event.screen_size[1],
		constants.camera.clip_dists.near, constants.camera.clip_dists.far, projection);

	glm_mul(projection, view, camera -> view_projection); // For billboard shader
	glm_mul(camera -> view_projection, (mat4) GLM_MAT4_IDENTITY_INIT, camera -> model_view_projection); // For sector shader
}

#endif
