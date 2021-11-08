#include "demo_4.c"

/*
- create rects through specifying origin vec3 and size vec3, that uses vbo indexing
- wrap textures around taller and wider rectangles
*/

void configurable_move(const GLuint shader_program, vec3 pos, mat4 view,
	mat4 view_times_projection, mat4 model_view_projection, const byte set_up_mvp) {

	static GLfloat hori_angle = (GLfloat) M_PI, vert_angle = 0.0f, last_time;

	static byte first_call = 1;
	if (first_call) {
		last_time = SDL_GetTicks() / 1000.0f;
		first_call = 0;
		return;
	}

	/*
	const GLfloat
		move_speed = 3.0f,
		look_speed = 0.08f,
		half_pi = (GLfloat) M_PI / 2.0f;
	*/

	int mouse_dx, mouse_dy;
	SDL_GetRelativeMouseState(&mouse_dx, &mouse_dy);

	const GLfloat delta_time = (SDL_GetTicks() / 1000.0f) - last_time;
	hori_angle += constants.speeds.look * delta_time * -mouse_dx;
	vert_angle += constants.speeds.look * delta_time * -mouse_dy;

	if (vert_angle > constants.numbers.half_pi) vert_angle = constants.numbers.half_pi;
	else if (vert_angle < -constants.numbers.half_pi) vert_angle = -constants.numbers.half_pi;

	const GLfloat cos_vert = cosf(vert_angle);
	vec3 direction = {cos_vert * sinf(hori_angle), sinf(vert_angle), cos_vert * cosf(hori_angle)};

	const GLfloat hori_angle_minus_half_pi = hori_angle - constants.numbers.half_pi, actual_speed = delta_time * constants.speeds.move;

	vec3 right = {sinf(hori_angle_minus_half_pi), 0.0f, cosf(hori_angle_minus_half_pi)};

	if (keys[SDL_SCANCODE_W]) glm_vec3_muladds(direction, actual_speed, pos);
	if (keys[SDL_SCANCODE_S]) glm_vec3_muladds(direction, -actual_speed, pos);
	if (keys[SDL_SCANCODE_A]) glm_vec3_muladds(right, -actual_speed, pos);
	if (keys[SDL_SCANCODE_D]) glm_vec3_muladds(right, actual_speed, pos);
	// if (pos[1] < 0.0f) pos[1] = 0.0f;

	//////////

	vec3 pos_plus_dir, up;
	glm_vec3_add(pos, direction, pos_plus_dir);
	glm_vec3_cross(right, direction, up);

	demo_2_configurable_matrix_setup(shader_program, pos, pos_plus_dir, up, view,
		view_times_projection, model_view_projection, set_up_mvp);

	if (keys[KEY_PRINT_POSITION])
		printf("pos = {%lf, %lf, %lf}\n", (double) pos[0], (double) pos[1], (double) pos[2]);

	last_time = SDL_GetTicks() / 1000.0f;
}

void move(const GLuint shader_program) {
	static vec3 pos = {1.5f, 1.5f, 3.5f};
	mat4 view, view_times_projection, model_view_projection;
	configurable_move(shader_program, pos, view, view_times_projection, model_view_projection, 1);
}

void demo_5_drawer(const StateGL* const sgl) {
	move(sgl -> shader_program);
	demo_4_core_drawer(12);
}

#ifdef DEMO_5
int main(void) {
	make_application(demo_5_drawer, demo_4_init, deinit_demo_vars);
}
#endif
