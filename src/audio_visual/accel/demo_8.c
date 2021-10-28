#include "demo_7.c"

/*
- optimized plane drawing
- want a solid base before continuing
- make data int8s and int16s
*/

/*
0__1
|  /
| /
2/
*/

enum {interleaved_plane_floats = 20};
enum {interleaved_plane_bytes = interleaved_plane_floats * sizeof(GLfloat)};

const char* const demo_8_vertex_shader =
	"#version 330 core\n"
	"layout (location = 0) in vec3 vertex_pos_model_space;\n"
	"layout (location = 1) in vec2 vertexUV;\n"

	"out vec2 UV;\n"
	"uniform mat4 MVP;\n"

	"void main() {\n"
		"gl_Position = MVP * vec4(vertex_pos_model_space, 1);\n"
		"UV = vertexUV;\n"
	"}\n";

void create_vert_plane_interleaved(const ivec3 top_left_corner,
	const int width, const int height, GLfloat* const plane_buffer) {

	const GLfloat left_x = top_left_corner[0], top_y = top_left_corner[1], z = top_left_corner[2];
	const GLfloat right_x = left_x + width, bottom_y = top_y - height;

	const GLfloat vertices_with_UV[interleaved_plane_floats] = {
		left_x, top_y, z, 0.0f, 0.0f, // last two floats are UV
		right_x, top_y, z, width, 0.0f,

		left_x, bottom_y, z, 0.0f, height,
		right_x, bottom_y, z, width, height
	};

	memcpy(plane_buffer, vertices_with_UV, interleaved_plane_bytes);
}

StateGL demo_8_init(void) {
	StateGL sgl;
	sgl.vertex_array = init_vao();

	enum {width = 3, height = 5};
	const ivec3 top_left_corner = {4, 4, 4};

	GLfloat* const plane_data = malloc(interleaved_plane_bytes);
	create_vert_plane_interleaved(top_left_corner, width, height, plane_data);

	//////////
	sgl.num_vertex_buffers = 1;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers, plane_data, interleaved_plane_bytes);

	enum {interleaved_vertex_bytes = 5 * sizeof(GLfloat)};

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, interleaved_vertex_bytes, NULL);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, interleaved_vertex_bytes, (void*) (3 * sizeof(GLfloat)));
	free(plane_data);
	//////////

	sgl.shader_program = init_shader_program(demo_8_vertex_shader, demo_4_fragment_shader);
	sgl.num_textures = 1;
	sgl.textures = init_textures(sgl.num_textures, "assets/walls/cobblestone.bmp");
	select_texture_for_use(sgl.textures[0], sgl.shader_program);
	enable_all_culling();

	return sgl;
}

void demo_8_drawer(const StateGL sgl) {
	move(sgl.shader_program);
	glClearColor(0.4f, 0.0f, 0.0f, 0.0f); // Dark blue
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

#ifdef DEMO_8
int main(void) {
	make_application(demo_8_drawer, demo_8_init, deinit_demo_vars);
}
#endif
