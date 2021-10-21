#include "utils.c"
#include "demo_1.c"

DemoVars demo_2_init(void) {
	DemoVars dv;
	demo_1_init_vertex_data(&dv);

	const char* const vertex_shader =
		"#version 330 core\n" // 140 -> OpenGL 3.1 (but that shader wouldn't compile)
		"layout(location = 0) in vec3 vertexPosition_modelspace;\n"
		"uniform mat4 MVP;\n"
		"void main() {\n"
			"gl_Position = MVP * vec4(vertexPosition_modelspace, 1);\n"
		"}\n",

	*const fragment_shader =
		"#version 330 core\n"
		"out vec3 color;\n"
		"void main() {\n"
			"color = vec3(0, 0, 1);\n" // Blue
		"}\n";

	dv.shader_program = init_shader_program(vertex_shader, fragment_shader);

	return dv;
}

double to_radians(const double degrees) {
	return degrees * M_PI / 180.0;
}

void demo_2_drawer(const DemoVars dv) {
	mat4
		projection, view, view_times_model, model_view_projection,
		model = GLM_MAT4_IDENTITY_INIT;

	vec3 camera_pos = {4.0f, 3.0f, 3.0f}, origin = {0.0f, 0.0f, 0.0f}, up = {0.0f, 1.0f, 0.0f};

	glm_perspective(to_radians(90.0), (float) SCR_W / SCR_H, 0.1f, 100.0f, projection);
	glm_lookat(camera_pos, origin, up, view);

	glm_mul(view, model, view_times_model);
	glm_mul(projection, view_times_model, model_view_projection);

	const GLuint matrix_id = glGetUniformLocation(dv.shader_program, "MVP");
	glUniformMatrix4fv(matrix_id, 1, GL_FALSE, &model_view_projection[0][0]);

	demo_1_drawer(dv);
}

/*
int main(void) {
	make_application(demo_2_drawer, demo_2_init, demo_1_deinit);
}
*/
