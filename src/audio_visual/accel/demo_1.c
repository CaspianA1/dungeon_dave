#include "utils.c"

// how to make the shader work?

DemoVars triangle_init(void) {
	DemoVars dv;

	static const GLfloat triangle_data[] = {
		-1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		0.0f, 1.0f, 0.0f
	};

	glGenVertexArrays(1, &dv.vertex_array);
	glBindVertexArray(dv.vertex_array);

	glGenBuffers(1, &dv.vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, dv.vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_data), triangle_data, GL_STATIC_DRAW);

	//////////

	const char* const vertex_shader =
		"#version 330 core\n" // 140 -> OpenGL 3.1 (but that shader wouldn't compile)
		"layout(location = 0) in vec3 vertexPosition_modelspace;\n"
		"void main() {\n"
			"gl_Position.xyz = vertexPosition_modelspace;\n"
			"gl_Position.w = 1.0;\n"
		"}\n",
	
	*const fragment_shader =
		"#version 330 core\n"
		"out vec3 color;\n"
		"void main() {\n"
			"color = vec3(1,0,0);\n" // Red
		"}\n";

	dv.shader_program = init_shader_program(vertex_shader, fragment_shader);
	glUseProgram(dv.shader_program);

	return dv;
}

void triangle_deinit(const DemoVars dv) {
	glDeleteProgram(dv.shader_program);
	glDeleteBuffers(1, &dv.vertex_buffer);
	glDeleteVertexArrays(1, &dv.vertex_array);
}

void triangle_drawer(const DemoVars dv) {
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, dv.vertex_buffer);

	glVertexAttribPointer(
   		0, 3, GL_FLOAT, // attribute 0, 3 vertices, type
   		GL_FALSE, 0, // not normalized, stride
		NULL // array buffer offset
	);

	glDrawArrays(GL_TRIANGLES, 0, 3); // Starting from vertex 0; 3 vertices total -> 1 triangle
	glDisableVertexAttribArray(0);
}

int main(void) {
	make_application(triangle_drawer, triangle_init, triangle_deinit);
}
