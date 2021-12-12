#ifndef CAMERA_C
#define CAMERA_C

#include "headers/camera.h"
#include "headers/constants.h"

Event get_next_event(void) {
	static GLint viewport_size[4];
	glGetIntegerv(GL_VIEWPORT, viewport_size);

	Event e = {
		.movement_bits =
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
	memset(camera, 0, sizeof(Camera));
	memcpy(camera -> pos, init_pos, sizeof(vec3));
	camera -> fov = constants.init_fov;
}

static void update_camera(Camera* const camera, const Event event) {
	static GLfloat last_time;
	static byte first_call = 1;

	if (first_call) {
		last_time = SDL_GetTicks() / 1000.0f;
		first_call = 0;
		return;
	}

	camera -> aspect_ratio = (GLfloat) event.screen_size[0] / event.screen_size[1];

	const GLfloat delta_time = (SDL_GetTicks() / 1000.0f) - last_time;
	camera -> hori_angle += constants.speeds.look * delta_time * -event.mouse_dx;
	camera -> vert_angle += constants.speeds.look * delta_time * -event.mouse_dy;

	if (camera -> vert_angle > constants.max_vert_angle) camera -> vert_angle = constants.max_vert_angle;
	else if (camera -> vert_angle < -constants.max_vert_angle) camera -> vert_angle = -constants.max_vert_angle;

	const GLfloat
		cos_vert = cosf(camera -> vert_angle),
		hori_angle_minus_half_pi = camera -> hori_angle - (GLfloat) M_PI_2,
		actual_speed = delta_time * constants.speeds.move;

	vec3 dir = {cos_vert * sinf(camera -> hori_angle), sinf(camera -> vert_angle), cos_vert * cosf(camera -> hori_angle)};
	memcpy(camera -> dir, dir, sizeof(vec3));

	camera -> right_xz[0] = sinf(hori_angle_minus_half_pi);
	camera -> right_xz[1] = cosf(hori_angle_minus_half_pi);

	vec3 right = {camera -> right_xz[0], 0.0f, camera -> right_xz[1]};

	vec3 pos;
	memcpy(pos, camera -> pos, sizeof(vec3));

	// Forward, backward, left, right
	if (event.movement_bits & 1) glm_vec3_muladds(dir, actual_speed, pos);
	if (event.movement_bits & 2) glm_vec3_muladds(dir, -actual_speed, pos);
	if (event.movement_bits & 4) glm_vec3_muladds(right, -actual_speed, pos);
	if (event.movement_bits & 8) glm_vec3_muladds(right, actual_speed, pos);

	if (keys[KEY_PRINT_POSITION])
		printf("pos = {%lf, %lf, %lf}\n", (double) pos[0], (double) pos[1], (double) pos[2]);

	//////////

	vec3 rel_origin, up;
	glm_vec3_add(pos, dir, rel_origin);
	glm_vec3_cross(right, dir, up);

	mat4 view, projection;
	glm_lookat(pos, rel_origin, up, view);

	glm_perspective(camera -> fov, camera -> aspect_ratio,
		constants.clip_dists.near, constants.clip_dists.far, projection);

	glm_mul(projection, view, camera -> view_projection); // For billboard shader
	glm_mul(camera -> view_projection, (mat4) GLM_MAT4_IDENTITY_INIT, camera -> model_view_projection); // For sector shader

	memcpy(camera -> pos, pos, sizeof(vec3));
	last_time = SDL_GetTicks() / 1000.0f;
}

#endif
