#include "demo_2.c"
#define deinit_texture(t) glDeleteTextures(1, &t)

StateGL demo_3_init(void) {
	StateGL sgl;

	enum {points_per_triangle = 9, triangles_per_cube = 12};
	enum {num_points = points_per_triangle * triangles_per_cube};

	/*
	For a cube:
	- 6 faces
	- 2 triangles per face
	- 12 triangles
	- 9 points per triangle
	*/

	static const GLfloat vertex_data[num_points] = {
		-1.0f, -1.0f, -1.0f, // triangle 1: begin
		-1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f, // triangle 1: end
		1.0f, 1.0f, -1.0f, // triangle 2: begin
		-1.0f, -1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f, // triangle 2: end
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
	},

	color_data[num_points] = {
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
		vertex_data, num_points,
		color_data, num_points);

	//////////

	const char* const vertex_shader =
		"#version 330 core\n"
		"layout(location = 0) in vec3 vertexPosition_modelspace;\n"
		"layout(location = 1) in vec3 vertexColor;\n"
		"out vec3 fragmentColor;\n"
		"uniform mat4 MVP;\n"
		"void main() {\n"
			"gl_Position =  MVP * vec4(vertexPosition_modelspace, 1);"
			"fragmentColor = vertexColor;\n"
		"}\n",

	*const fragment_shader =
		"#version 330 core\n"
		"in vec3 fragmentColor;\n"
		"out vec3 color;\n"
		"void main() {\n"
			"color = fragmentColor;\n"
		"}\n";
	
	sgl.shader_program = init_shader_program(vertex_shader, fragment_shader);

	glClearColor(0.0f, 0.0f, 0.4f, 0.0f); // Dark blue
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	return sgl;
}

void demo_3_drawer(const StateGL sgl) {
	vec3 camera_pos = {4.0f, 3.0f, -3.0f};
	demo_2_matrix_setup(sgl.shader_program, camera_pos);

	bind_vbos_to_vao(sgl.vertex_buffers, sgl.num_vertex_buffers);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	unbind_vbos_from_vao(sgl.num_vertex_buffers);
}

int main(void) {
	make_application(demo_3_drawer, demo_3_init, deinit_demo_vars);
}
