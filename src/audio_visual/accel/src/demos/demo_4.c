#include "demo_3.c"
#include "../data/shaders.c"

/*
vert_1: on zy (2, 1)
vert_2: on xy (0, 1)
flat:   on zx (2, 0)

vert 1 masks: (0, 1), (1, 1)
vert 2 masks: (0, 1), (1, 1)
flat mask: (0, 1)

expanded:
	Correct for first vert_1 wall side: (z, 1 - y)
	"UV = vec2(vertex_pos_model_space.z, 1.0f - vertex_pos_model_space.y);\n"

	Correct for second vert_1 wall side: (1 - z, 1 - y)
	"UV = 1.0f - vertex_pos_model_space.zy;\n"

	Correct for first vert_2 wall side: (x, 1 - y)
	"UV = vec2(vertex_pos_model_space.x, 1.0f - vertex_pos_model_space.y);\n"

	Correct for second vert_2 wall side: (1 - x, 1 - y)
	"UV = 1.0f - vec2(vertex_pos_model_space.x, vertex_pos_model_space.y);\n"

	Correct for floors: (z, 1 - x)
	"UV = vec2(vertex_pos_model_space.z, 1.0f - vertex_pos_model_space.x);\n"
*/

StateGL demo_4_init(void) {
	StateGL sgl;

	sgl.vertex_array = init_vao();

	enum {floats_per_uv = 2, uvs_per_triangle = 3};
	enum {num_uv_floats = triangles_per_cube * uvs_per_triangle * floats_per_uv};

	#define FIRST_UV 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
	#define SECOND_UV 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f,
	#define THIRD_UV 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
	#define FOURTH_UV 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
	#define FIFTH_UV 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
	#define SIXTH_UV 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,

	const GLfloat uv_data[] = {
		// FIRST_UV SECOND_UV THIRD_UV FOURTH_UV FIFTH_UV SIXTH_UV
		FIRST_UV SECOND_UV SECOND_UV THIRD_UV
		FOURTH_UV THIRD_UV THIRD_UV SECOND_UV
		FIFTH_UV FIRST_UV FOURTH_UV SIXTH_UV
	};

	sgl.num_vertex_buffers = 2;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers,
		demo_3_vertex_data, sizeof(demo_3_vertex_data),
		uv_data, sizeof(uv_data));

	bind_vbos_to_vao(sgl.vertex_buffers, sgl.num_vertex_buffers, 3, 2);

	sgl.shader_program = init_shader_program(sector_vertex_shader, sector_fragment_shader);
	glUseProgram(sgl.shader_program);

	sgl.num_textures = 1;
	sgl.textures = init_textures(sgl.num_textures, "../../../assets/walls/hieroglyph.bmp", tex_nonrepeating);
	select_texture_for_use(sgl.textures[0], sgl.shader_program);

	// For textures with an alpha channel, enable this
	/* glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); */

	enable_all_culling();

	return sgl;
}

void demo_4_core_drawer(const int num_triangles) {
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f); // Dark blue
	draw_triangles(num_triangles);
}

void demo_4_drawer(const StateGL* const sgl) {
	static vec3 camera_pos = {2.0f, 2.0f, 0.0f};
	const GLfloat step = 0.05;
	if (keys[SDL_SCANCODE_W]) camera_pos[0] += step;
	if (keys[SDL_SCANCODE_S]) camera_pos[0] -= step;
	if (keys[SDL_SCANCODE_1]) camera_pos[1] += step;
	if (keys[SDL_SCANCODE_2]) camera_pos[1] -= step;
	if (keys[SDL_SCANCODE_A]) camera_pos[2] += step;
	if (keys[SDL_SCANCODE_D]) camera_pos[2] -= step;

	demo_2_matrix_setup(sgl -> shader_program, camera_pos);
	demo_4_core_drawer(12);
}

#ifdef DEMO_4
int main(void) {
	make_application(demo_4_drawer, demo_4_init, deinit_demo_vars);
}
#endif
