#include "demo_2.c"

enum {points_per_triangle = 9, triangles_per_cube = 12};
enum {cube_num_points = points_per_triangle * triangles_per_cube};

/*
#define A -1.000000, -1.000000, -1.000000,
#define B -1.000000, -1.000000, 1.000000,
#define C -1.000000, 1.000000, 1.000000,
#define D 1.000000, 1.000000, -1.000000,
#define E -1.000000, 1.000000, -1.000000,
#define F 1.000000, -1.000000, 1.000000,
#define G 1.000000, -1.000000, -1.000000,
#define H 1.000000, 1.000000, 1.000000,
*/

	/*
	A B C
	D A E
	F A G
	D G A
	A C E
	F B A
	C B F
	H G D
	G H F
	H D E
	H E C
	H C F
	*/

const GLfloat demo_3_vertex_data[] = {
	// A B C D E F G H
	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f, 1.0f,
	-1.0f, 1.0f, 1.0f,

	1.0f, 1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f, 1.0f, -1.0f,

	1.0f, -1.0f, 1.0f,
	-1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,

	1.0f, 1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,

	-1.0f, -1.0f, -1.0f,
	-1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f, -1.0f,

	1.0f, -1.0f, 1.0f,
	-1.0f, -1.0f, 1.0f,
	-1.0f, -1.0f, -1.0f,

	-1.0f, 1.0f, 1.0f,
	-1.0f, -1.0f, 1.0f,
	1.0f, -1.0f, 1.0f,

	1.0f, 1.0f, 1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f, 1.0f, -1.0f,

	1.0f, -1.0f, -1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, -1.0f, 1.0f,

	1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, -1.0f,
	-1.0f, 1.0f, -1.0f,

	1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f, -1.0f,
	-1.0f, 1.0f, 1.0f,

	1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f, 1.0f,
	1.0f, -1.0f, 1.0f
};

StateGL demo_3_init(void) {
	StateGL sgl;

	/*
	For a cube:
	- 6 faces
	- 2 triangles per face
	- 12 triangles
	- 9 points per triangle
	*/

	const GLfloat color_data[cube_num_points] = {
		0.583f, 0.771f, 0.014f,
		0.609f, 0.115f, 0.436f,
		0.327f, 0.483f, 0.844f,
		0.822f, 0.569f, 0.201f,
		0.435f, 0.602f, 0.223f,
		0.310f, 0.747f, 0.185f,
		0.597f, 0.770f, 0.761f,
		0.559f, 0.436f, 0.730f,
		0.359f, 0.583f, 0.152f,
		0.483f, 0.596f, 0.789f,
		0.559f, 0.861f, 0.639f,
		0.195f, 0.548f, 0.859f,
		0.014f, 0.184f, 0.576f,
		0.771f, 0.328f, 0.970f,
		0.406f, 0.615f, 0.116f,
		0.676f, 0.977f, 0.133f,
		0.971f, 0.572f, 0.833f,
		0.140f, 0.616f, 0.489f,
		0.997f, 0.513f, 0.064f,
		0.945f, 0.719f, 0.592f,
		0.543f, 0.021f, 0.978f,
		0.279f, 0.317f, 0.505f,
		0.167f, 0.620f, 0.077f,
		0.347f, 0.857f, 0.137f,
		0.055f, 0.953f, 0.042f,
		0.714f, 0.505f, 0.345f,
		0.783f, 0.290f, 0.734f,
		0.722f, 0.645f, 0.174f,
		0.302f, 0.455f, 0.848f,
		0.225f, 0.587f, 0.040f,
		0.517f, 0.713f, 0.338f,
		0.053f, 0.959f, 0.120f,
		0.393f, 0.621f, 0.362f,
		0.673f, 0.211f, 0.457f,
		0.820f, 0.883f, 0.371f,
		0.982f, 0.099f, 0.879f
	};

	sgl.vertex_array = init_vao();

	sgl.num_vertex_buffers = 2;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers,
		demo_3_vertex_data, sizeof(demo_3_vertex_data),
		color_data, sizeof(color_data));

	bind_vbos_to_vao(sgl.vertex_buffers, (GLuint) sgl.num_vertex_buffers, 3, 3);

	sgl.num_textures = 0;

	//////////

	const GLchar* const vertex_shader =
		"#version 330 core\n"

		"layout(location = 0) in vec3 vertex_pos_model_space;\n"
		"layout(location = 1) in vec3 vertex_color;\n"
		"out vec3 fragment_color;\n"

		"uniform mat4 model_view_projection;\n"
		"void main(void) {\n"
			"gl_Position = model_view_projection * vec4(vertex_pos_model_space, 1);"
			"fragment_color = vertex_color;\n"
		"}\n",

	*const fragment_shader =
		"#version 330 core\n"

		"in vec3 fragment_color;\n"
		"out vec3 color;\n"

		"void main(void) {\n"
			"color = fragment_color;\n"
		"}\n";

	sgl.shader_program = init_shader_program(vertex_shader, fragment_shader);
	use_shader_program(sgl.shader_program);

	enable_all_culling();

	vec3 camera_pos = {4.0f, 3.0f, -3.0f};
	demo_2_matrix_setup(sgl.shader_program, camera_pos);
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f); // Dark blue

	return sgl;
}

void demo_3_drawer(const StateGL* const sgl) {
	(void) sgl;
	enum {num_triangles = 12};
	glDrawArrays(GL_TRIANGLES, 0, num_triangles * 3);
}

#ifdef DEMO_3
int main(void) {
	make_application(demo_3_drawer, demo_3_init, deinit_demo_vars);
}
#endif
