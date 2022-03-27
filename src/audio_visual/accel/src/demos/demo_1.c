#include "../utils.c"

// SDL2, SDL2_ttf, SDL2_mixer, OpenGL, glad, cglm

void demo_1_init_vertex_data(StateGL* const sgl) {
	const GLfloat triangle_data[9] = {
		-1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		0.0f, 1.0f, 0.0f
	};

	sgl -> vertex_array = init_vao();
	sgl -> num_vertex_buffers = 1;
	sgl -> vertex_buffers = init_vbos(sgl -> num_vertex_buffers, triangle_data, sizeof(triangle_data));
	bind_vbos_to_vao(sgl -> vertex_buffers, (GLuint) sgl -> num_vertex_buffers, 3);
}

StateGL demo_1_init(void) {
	StateGL sgl;
	demo_1_init_vertex_data(&sgl);
	sgl.num_textures = 0;

	const GLchar* const vertex_shader =
		"#version 330 core\n" // 140 -> OpenGL 3.1 (but that shader wouldn't compile)

		"layout(location = 0) in vec3 vertex_pos_world_space;\n"
		"void main(void) {\n"
			"gl_Position.xyz = vertex_pos_world_space;\n"
			"gl_Position.w = 1.0;\n"
		"}\n",

	*const fragment_shader =
		"#version 330 core\n"

		"out vec3 color;\n"
		"void main(void) {\n"
			"color = vec3(1, 0, 0);\n" // Red
		"}\n";

	sgl.shader_program = init_shader_program(vertex_shader, fragment_shader);
	use_shader_program(sgl.shader_program);

	return sgl;
}

void demo_1_drawer(const StateGL* const sgl) {
	(void) sgl;
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

#ifdef DEMO_1
int main(void) {
	make_application(demo_1_drawer, demo_1_init, deinit_demo_vars);
}
#endif
