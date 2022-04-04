#include "demo_1.c"
#include "../headers/constants.h"

void demo_2_configurable_matrix_setup(const GLuint shader,
	vec3 pos, vec3 rel_origin, vec3 up, mat4 view, mat4 view_times_projection, mat4 model_view_projection, const bool set_up_mvp) {

	mat4 projection, model = GLM_MAT4_IDENTITY_INIT, view_times_model;

	glm_perspective(constants.camera.init.fov, (GLfloat) WINDOW_W / WINDOW_H,
		constants.camera.clip_dists.near, constants.camera.clip_dists.default_far, projection);

	glm_lookat(pos, rel_origin, up, view);

	glm_mul(view, projection, view_times_projection); // For external usage

	glm_mul(view, model, view_times_model);
	glm_mul(projection, view_times_model, model_view_projection);

	if (set_up_mvp) {
		static GLint model_view_projection_id;
		static bool first_call = true;

		if (first_call) {
			INIT_UNIFORM(model_view_projection, shader);
			first_call = false;
		}

		UPDATE_UNIFORM(model_view_projection, Matrix4fv, 1, GL_FALSE, &model_view_projection[0][0]);
	}
}

void demo_2_matrix_setup(const GLuint shader, vec3 camera_pos) {
	vec3 origin = {0.0f, 0.0f, 0.0f}, up = {0.0f, 1.0f, 0.0f};
	mat4 view, view_times_model, model_view_projection;
	demo_2_configurable_matrix_setup(shader, camera_pos, origin, up, view, view_times_model, model_view_projection, true);
}

StateGL demo_2_init(void) {
	StateGL sgl;
	demo_1_init_vertex_data(&sgl);
	sgl.num_textures = 0;

	const GLchar* const vertex_shader =
		"#version 330 core\n"

		"layout(location = 0) in vec3 vertex_pos_world_space;\n"
		"uniform mat4 model_view_projection;\n"
		"void main(void) {\n"
			"gl_Position = model_view_projection * vec4(vertex_pos_world_space, 1);\n"
		"}\n",

	*const fragment_shader =
		"#version 330 core\n"
		"out vec3 color;\n"
		"void main(void) {\n"
			"color = vec3(0, 0, 1);\n" // Blue
		"}\n";

	sgl.shader = init_shader(vertex_shader, fragment_shader);
	use_shader(sgl.shader);

	vec3 camera_pos = {4.0f, 3.0f, 3.0f};
	demo_2_matrix_setup(sgl.shader, camera_pos);

	return sgl;
}

#ifdef DEMO_2
int main(void) {
	make_application(demo_1_drawer, demo_2_init, deinit_demo_vars);
}
#endif
