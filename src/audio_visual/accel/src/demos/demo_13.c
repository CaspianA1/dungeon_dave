#include "demo_11.c"
#include "../headers/constants.h"

const GLchar* const demo_13_billboard_vertex_shader =
	"#version 330 core\n"

	"out vec2 UV;\n"

	"uniform vec2 billboard_size_world_space, right_xz_world_space;\n"
	"uniform vec3 billboard_center_world_space;\n"
	"uniform mat4 view_projection;\n"

	"const vec2 vertices_model_space[4] = vec2[4](\n"
		"vec2(-0.5f, -0.5f), vec2(0.5f, -0.5f),\n"
		"vec2(-0.5f, 0.5f), vec2(0.5f, 0.5f)\n"
	");\n"

	"const vec3 up_world_space = vec3(0.0f, 1.0f, 0.0f);\n"

	"void main(void) {\n"
		"vec2 vertex_model_space = vertices_model_space[gl_VertexID];\n"
		"vec2 corner_world_space = vertex_model_space * billboard_size_world_space;\n"

		"vec3 vertex_world_space = billboard_center_world_space +\n"
			"corner_world_space.x * vec3(right_xz_world_space, 0.0f).xzy\n"
			"+ corner_world_space.y * up_world_space;\n"

		"gl_Position = view_projection * vec4(vertex_world_space, 1.0f);\n"
		"UV = vec2(vertex_model_space.x, -vertex_model_space.y) + 0.5f;\n"
	"}\n",

*const demo_13_billboard_fragment_shader =
    "#version 330 core\n"

	"in vec2 UV;\n"

	"out vec4 color;\n"

	"uniform sampler2D texture_sampler;\n"

	"void main(void) {\n"
		"color = texture(texture_sampler, UV);\n"
	"}\n";

void demo_13_move(vec3 pos, vec3 right, mat4 view_times_projection, const GLuint shader_program) {
	static GLfloat hori_angle = PI, vert_angle = 0.0f, last_time;

	static byte first_call = 1;
	if (first_call) {
		last_time = SDL_GetTicks() / 1000.0f;
		first_call = 0;
		return;
	}

	const GLfloat move_speed = 3.0f, look_speed = 0.08f;

	int mouse_dx, mouse_dy;
	SDL_GetRelativeMouseState(&mouse_dx, &mouse_dy);

	const GLfloat delta_time = (SDL_GetTicks() / 1000.0f) - last_time;
	hori_angle += look_speed * delta_time * -mouse_dx; // dx and dy are global from demo_5.c
	vert_angle += look_speed * delta_time * -mouse_dy;

	if (vert_angle > HALF_PI) vert_angle = HALF_PI;
	else if (vert_angle < -HALF_PI) vert_angle = -HALF_PI;

	const GLfloat cos_vert = cosf(vert_angle);
	vec3 direction = {cos_vert * sinf(hori_angle), sinf(vert_angle), cos_vert * cosf(hori_angle)};

	const GLfloat hori_angle_minus_half_pi = hori_angle - HALF_PI, actual_speed = delta_time * move_speed;

	vec3 _right = {sinf(hori_angle_minus_half_pi), 0.0f, cosf(hori_angle_minus_half_pi)};
	memcpy(right, _right, sizeof(vec3));

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
	mat4 projection, view, model_view_projection, view_times_model, model = GLM_MAT4_IDENTITY_INIT;
	glm_perspective(constants.camera.init.fov, (GLfloat) WINDOW_W / WINDOW_H,
		constants.camera.clip_dists.near, constants.camera.clip_dists.far, projection);
	glm_lookat(pos, pos_plus_dir, up, view);
	glm_mul(projection, view, view_times_projection); // For external usage

	glm_mul(view, model, view_times_model);
	glm_mul(projection, view_times_model, model_view_projection);
	//////////

	if (keys[KEY_PRINT_POSITION])
		printf("pos = {%lf, %lf, %lf}\n", (double) pos[0], (double) pos[1], (double) pos[2]);

	extern GLuint sector_shader;
	glUseProgram(sector_shader); // Binding sector_shader

	static GLint model_view_projection_id;
	static byte second_call = 1; // Not first_call b/c that is defined above
	if (second_call) {
		INIT_UNIFORM(model_view_projection, sector_shader);
		second_call = 0;
	}

	UPDATE_UNIFORM(model_view_projection, Matrix4fv, 1, GL_FALSE, &model_view_projection[0][0]);
	glUseProgram(shader_program); // Binding billboard shader back

	last_time = SDL_GetTicks() / 1000.0f;
}

void demo_13_matrix_setup(const GLuint shader_program, const GLfloat center[3]) {
	static GLint right_xz_world_space_id, view_projection_id;
	static byte first_call = 1;

	glUseProgram(shader_program); // Enable billboard shader

	if (first_call) {
		INIT_UNIFORM(right_xz_world_space, shader_program);
		INIT_UNIFORM(view_projection, shader_program);
		INIT_UNIFORM_VALUE(billboard_size_world_space, shader_program, 2f, 1.0f, 1.0f);
		INIT_UNIFORM_VALUE(billboard_center_world_space, shader_program, 3fv, 1, center);
		first_call = 0;
	}

	static vec3 pos = {2.0f, 4.5f, 2.0f}, right;
	mat4 view_times_projection;
	demo_13_move(pos, right, view_times_projection, shader_program);

	UPDATE_UNIFORM(right_xz_world_space, 2f, right[0], right[2]);
	UPDATE_UNIFORM(view_projection, Matrix4fv, 1, GL_FALSE, &view_times_projection[0][0]);
}

GLuint sector_shader;
GLfloat center[3] = {5.5f, 4.5f, 8.5f};

StateGL demo_13_init(void) {
	StateGL sgl = {.vertex_array = init_vao()};

	const mesh_type_t origin[3] = {1, 4, 1}, size[3] = {5, 4, 8};
	mesh_type_t* const cuboid_mesh = malloc(bytes_per_mesh);
	create_sector_mesh(origin, size, cuboid_mesh);

	//////////
	sgl.num_vertex_buffers = 1;
	sgl.vertex_buffers = malloc(sgl.num_vertex_buffers * sizeof(GLuint));

	glGenBuffers(sgl.num_vertex_buffers, sgl.vertex_buffers);
	glBindBuffer(GL_ARRAY_BUFFER, sgl.vertex_buffers[0]);
	glBufferData(GL_ARRAY_BUFFER, bytes_per_mesh, cuboid_mesh, GL_STATIC_DRAW);

	free(cuboid_mesh);
	//////////

	sgl.num_textures = 2;
	sgl.textures = init_plain_textures(sgl.num_textures,
		"../../../../assets/objects/tomato.bmp", TexNonRepeating,
		"../../../../assets/walls/saqqara.bmp", TexRepeating);

	sgl.shader_program = init_shader_program(demo_13_billboard_vertex_shader, demo_13_billboard_fragment_shader);
	glUseProgram(sgl.shader_program);
	use_texture(sgl.textures[0], sgl.shader_program, "texture_sampler", TexPlain, BILLBOARD_TEXTURE_UNIT);

	sector_shader = init_shader_program(demo_4_vertex_shader, demo_4_fragment_shader);
	glUseProgram(sector_shader);
	use_texture(sgl.textures[1], sector_shader, "texture_sampler", TexPlain, SECTOR_TEXTURE_UNIT);

	enable_all_culling();
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	return sgl;
}

void demo_13_drawer(const StateGL* const sgl) {
	glClearColor(0.2f, 0.8f, 0.5f, 0.0f); // Barf green
	demo_13_matrix_setup(sgl -> shader_program, center);

	// Drawing non-transparent objects first

	bind_sector_mesh_to_vao();

	glUseProgram(sector_shader);
	glBindBuffer(GL_ARRAY_BUFFER, sgl -> vertex_buffers[0]);
	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glDrawArrays(GL_TRIANGLES, 0, triangles_per_mesh * 3);

	unbind_sector_mesh_from_vao();

	//////////

	glUseProgram(sgl -> shader_program);
	glEnable(GL_BLEND); // Turning on alpha blending for drawing billboards
	glDisable(GL_CULL_FACE);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void demo_13_deinit(const StateGL* const sgl) {
	glDeleteProgram(sector_shader);
	deinit_demo_vars(sgl);
}

#ifdef DEMO_13
int main(void) {
	make_application(demo_13_drawer, demo_13_init, demo_13_deinit);
}
#endif
