
#include "demo_10.c"

/*
- the code is messy
- make the billboard not turn when looking down or up
*/

typedef GLfloat billboard_type_t;
#define BILLBOARD_TYPE_ENUM GL_FLOAT

const char* const demo_13_vertex_shader =
	"#version 330 core\n"
	"layout(location = 0) in vec2 vertex_model_space;\n"

	"out vec2 UV;\n"

	"uniform vec2 billboard_size_world_space;\n"
	"uniform vec3 billboard_center_world_space, cam_right_world_space, cam_up_world_space;\n"
	"uniform mat4 VP;\n" // View-projection matrix

	"void main() {\n"
		"vec3 vertex_world_space = billboard_center_world_space \n"
			"+ cam_right_world_space * vertex_model_space.x * billboard_size_world_space.x\n"
			"+ cam_up_world_space * vertex_model_space.y * billboard_size_world_space.y;\n"

		"gl_Position = VP * vec4(vertex_world_space, 1.0f);\n"
		"UV = vec2(vertex_model_space.x, -vertex_model_space.y) + vec2(0.5f);\n"
	"}\n",

*const demo_13_fragment_shader =
	"#version 330 core\n"
	"in vec2 UV;\n"
	"out vec4 color;\n" // For textures with an alpha channel, enable 4 channels
	"uniform sampler2D texture_sampler;\n"
	"void main() {\n"
		"color = texture(texture_sampler, UV).rgba;\n"
	"}\n";

void demo_13_move(vec3 pos, mat4 view, mat4 view_times_projection, const GLuint shader_program) {
	static GLfloat hori_angle = (GLfloat) M_PI, vert_angle = 0.0f, last_time;

	static byte first_call = 1;
	if (first_call) {
		last_time = SDL_GetTicks() / 1000.0f;
		first_call = 0;
		return;
	}

	const GLfloat
		move_speed = 3.0f,
		look_speed = 0.08f,
		half_pi = (GLfloat) M_PI / 2.0f;

	SDL_GetRelativeMouseState(&mouse_dx, &mouse_dy);

	const GLfloat delta_time = (SDL_GetTicks() / 1000.0f) - last_time;
	hori_angle += look_speed * delta_time * -mouse_dx; // dx and dy are global from demo_5.c
	vert_angle += look_speed * delta_time * -mouse_dy;

	if (vert_angle > half_pi) vert_angle = half_pi;
	else if (vert_angle < -half_pi) vert_angle = -half_pi;

	const GLfloat cos_vert = cosf(vert_angle);
	vec3 direction = {cos_vert * sinf(hori_angle), sinf(vert_angle), cos_vert * cosf(hori_angle)};

	const GLfloat hori_angle_minus_half_pi = hori_angle - half_pi, actual_speed = delta_time * move_speed;

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
	//////////
	mat4 projection, model_view_projection, view_times_model, model = GLM_MAT4_IDENTITY_INIT;
	glm_perspective(to_radians(FOV), (GLfloat) SCR_W / SCR_H, near_clip_plane, far_clip_plane, projection);
	glm_lookat(pos, pos_plus_dir, up, view);
	glm_mul(projection, view, view_times_projection); // For external usage

	glm_mul(view, model, view_times_model);
	glm_mul(projection, view_times_model, model_view_projection);
	//////////

	if (keys[KEY_PRINT_POSITION])
		printf("pos = {%lf, %lf, %lf}\n", (double) pos[0], (double) pos[1], (double) pos[2]);

	extern GLuint poly_shader;
	glUseProgram(poly_shader); // Binding poly_shader

	static GLuint matrix_id;
	static byte second_call = 1; // Not first_call b/c that is defined above
	if (second_call) {
		matrix_id = glGetUniformLocation(poly_shader, "MVP");
		second_call = 0;
	}

	glUniformMatrix4fv(matrix_id, 1, GL_FALSE, &model_view_projection[0][0]);
	glUseProgram(shader_program); // Binding billboard shader back

	last_time = SDL_GetTicks() / 1000.0f;
}

void demo_13_matrix_setup(const GLuint shader_program, const billboard_type_t center[3], const billboard_type_t half_size[2]) {
	static GLint billboard_size_id, billboard_center_id, cam_up_id, cam_right_id, view_projection_matrix_id;
	static byte first_call = 1;

	glUseProgram(shader_program); // Enable billboard shader

	if (first_call) {
		billboard_size_id = glGetUniformLocation(shader_program, "billboard_size_world_space");
		billboard_center_id = glGetUniformLocation(shader_program, "billboard_center_world_space");

		cam_up_id = glGetUniformLocation(shader_program, "cam_up_world_space");
		cam_right_id = glGetUniformLocation(shader_program, "cam_right_world_space");
		view_projection_matrix_id = glGetUniformLocation(shader_program, "VP");

		glUniform3f(billboard_center_id, center[0], center[1], center[2]);
		glUniform2f(billboard_size_id, half_size[0] * 2.0f, half_size[1] * 2.0f);

		first_call = 0;
	}

	static vec3 pos = {1.5f, 1.5f, 3.5f};
	mat4 view, view_times_projection;
	demo_13_move(pos, view, view_times_projection, shader_program);

	glUniform3f(cam_right_id, view[0][0], view[1][0], view[2][0]); // Last to 0 for cool effect
	glUniform3f(cam_up_id, view[0][1], view[1][1], view[2][1]);

	glUniformMatrix4fv(view_projection_matrix_id, 1, GL_FALSE, &view_times_projection[0][0]);
}

const billboard_type_t center[3] = {5.5f, 4.5f, 8.5f}, half_size[2] = {0.5f, 0.5f};
GLuint poly_shader;

StateGL demo_13_init(void) {
	StateGL sgl = {.vertex_array = init_vao()};

	const billboard_type_t vertices[] = {
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, 0.5f, 0.0f
	};

	/////
	const plane_type_t origin[3] = {1, 4, 1}, size[3] = {5, 1, 8};
	plane_type_t* const flat_plane = malloc(bytes_per_plane);
	PLANE_CREATOR_NAME(hori)(origin, size[0], size[2], flat_plane);
	//////

	sgl.num_vertex_buffers = 2;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers, vertices, sizeof(vertices), flat_plane, bytes_per_plane);
	free(flat_plane);

	sgl.num_textures = 2;
	sgl.textures = init_textures(sgl.num_textures, "../../../assets/objects/jungle.bmp", "../../../assets/walls/saqqara.bmp");

	sgl.shader_program = init_shader_program(demo_13_vertex_shader, demo_13_fragment_shader);
	poly_shader = init_shader_program(demo_4_vertex_shader, demo_4_fragment_shader);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	enable_all_culling();
	return sgl;
}

void demo_13_drawer(const StateGL* const sgl) {
	// glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Black
	glClearColor(0.2f, 0.8f, 0.5f, 0.0f); // Barf green

	demo_13_matrix_setup(sgl -> shader_program, center, half_size);

	// Drawing non-transparent objects first
	glUseProgram(poly_shader);
	select_texture_for_use(sgl -> textures[1], poly_shader);
	glBindBuffer(GL_ARRAY_BUFFER, sgl -> vertex_buffers[1]);
	bind_interleaved_planes_to_vao();
	draw_triangles(2);

	// Turning on alpha blending for billboards
	glEnable(GL_BLEND);

	// Drawing billboard
	glUseProgram(sgl -> shader_program);
	select_texture_for_use(sgl -> textures[0], sgl -> shader_program);
	glBindBuffer(GL_ARRAY_BUFFER, sgl -> vertex_buffers[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, BILLBOARD_TYPE_ENUM, GL_FALSE, 0, NULL);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisable(GL_BLEND);
}

void demo_13_deinit(const StateGL* const sgl) {
	glDeleteProgram(poly_shader);
	deinit_demo_vars(sgl);
}

#ifdef DEMO_13
int main(void) {
	make_application(demo_13_drawer, demo_13_init, demo_13_deinit);
}
#endif
