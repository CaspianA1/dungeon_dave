#include "utils.c"
#include "demo_4.c"

/*
- vbo indexing
- create rects through specifying origin vec3 and size vec3
- wrap textures around taller and wider rectangles
*/

void move(const GLuint shader_program) {
	static float hori_angle = (float) M_PI, vert_angle = 0.0f, last_time;

	static byte first_call = 1;
	if (first_call) {
		last_time = SDL_GetTicks() / 1000.0f;
		first_call = 0;
		return;
	}

	const float
		move_speed = 3.0f,
		look_speed = 0.08f,
		half_pi = (float) M_PI / 2.0f,
		max_look_up_angle = (float) M_PI * 2.0f / 3.0f;

	int mouse_dx, mouse_dy;
	SDL_GetRelativeMouseState(&mouse_dx, &mouse_dy);

	const float delta_time = (SDL_GetTicks() / 1000.0f) - last_time;
	hori_angle += look_speed * delta_time * -mouse_dx;
	vert_angle += look_speed * delta_time * -mouse_dy;

	if (vert_angle > max_look_up_angle) vert_angle = max_look_up_angle;
	else if (vert_angle < -half_pi) vert_angle = -half_pi;

	const float cos_vert = cosf(vert_angle);
	vec3 direction = {cos_vert * sinf(hori_angle), sinf(vert_angle), cos_vert * cosf(hori_angle)};

	const float hori_angle_minus_half_pi = hori_angle - half_pi, actual_speed = delta_time * move_speed;
	vec3 right = {sinf(hori_angle_minus_half_pi), 0.0f, cosf(hori_angle_minus_half_pi)};

	static vec3 position = {0.0f, 0.0f, 1.5f};

	if (keys[SDL_SCANCODE_W]) glm_vec3_muladds(direction, actual_speed, position);
	if (keys[SDL_SCANCODE_S]) glm_vec3_muladds(direction, -actual_speed, position);
	if (keys[SDL_SCANCODE_A]) glm_vec3_muladds(right, -actual_speed, position);
	if (keys[SDL_SCANCODE_D]) glm_vec3_muladds(right, actual_speed, position);

	//////////

	vec3 pos_plus_dir, up;
	glm_vec3_add(position, direction, pos_plus_dir);
	glm_cross(right, direction, up);
	demo_2_configurable_matrix_setup(shader_program, position, pos_plus_dir, up);

	last_time = SDL_GetTicks() / 1000.0f;
}

void demo_5_drawer(const StateGL sgl) {
	move(sgl.shader_program);
	demo_4_core_drawer(sgl);
}

#ifdef DEMO_5
int main(void) {
	make_application(demo_5_drawer, demo_4_init, deinit_demo_vars);
}
#endif
