#include "demo_1.c"

void demo_2_configurable_matrix_setup(const GLuint shader_program,
	vec3 pos, vec3 rel_origin, vec3 up, mat4 view, mat4 view_times_projection, mat4 model_view_projection, const byte set_up_mvp) {

	mat4 projection, model = GLM_MAT4_IDENTITY_INIT, view_times_model;

	glm_perspective(to_radians(FOV), (GLfloat) SCR_W / SCR_H, constants.clip_dists.near, constants.clip_dists.far, projection);
	glm_lookat(pos, rel_origin, up, view);

	glm_mul(view, projection, view_times_projection); // For external usage

	glm_mul(view, model, view_times_model);
	glm_mul(projection, view_times_model, model_view_projection);

	if (set_up_mvp) {
		static GLuint matrix_id;
		static byte first_call = 1;

		if (first_call) {
			matrix_id = glGetUniformLocation(shader_program, "model_view_projection");
			first_call = 0;
		}

		glUniformMatrix4fv(matrix_id, 1, GL_FALSE, &model_view_projection[0][0]);
	}
}

void demo_2_matrix_setup(const GLuint shader_program, vec3 camera_pos) {
	vec3 origin = {0.0f, 0.0f, 0.0f}, up = {0.0f, 1.0f, 0.0f};
	mat4 view, view_times_model, model_view_projection;
	demo_2_configurable_matrix_setup(shader_program, camera_pos, origin, up, view, view_times_model, model_view_projection, 1);
}

StateGL demo_2_init(void) {
	StateGL sgl;
	demo_1_init_vertex_data(&sgl);
	sgl.num_textures = 0;

	const char* const vertex_shader =
		"#version 330 core\n"
		"layout(location = 0) in vec3 vertex_pos_model_space;\n"
		"uniform mat4 model_view_projection;\n"
		"void main() {\n"
			"gl_Position = model_view_projection * vec4(vertex_pos_model_space, 1);\n"
		"}\n",

	*const fragment_shader =
		"#version 330 core\n"
		"out vec3 color;\n"
		"void main() {\n"
			"color = vec3(0, 0, 1);\n" // Blue
		"}\n";

	sgl.shader_program = init_shader_program(vertex_shader, fragment_shader);

	vec3 camera_pos = {4.0f, 3.0f, 3.0f};
	demo_2_matrix_setup(sgl.shader_program, camera_pos);

	return sgl;
}

#ifdef DEMO_2
int main(void) {
	make_application(demo_1_drawer, demo_2_init, deinit_demo_vars);
}
#endif
