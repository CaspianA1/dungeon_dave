#include "demo_1.c"

static inline GLfloat to_radians(const GLfloat degrees) {
	return degrees * (GLfloat) M_PI / 180.0f;
}

void demo_2_configurable_matrix_setup(const GLuint shader_program,
	vec3 pos, vec3 rel_origin, vec3 up) {

	const GLfloat
		near_clip_plane = 0.1f,
		far_clip_plane = 441.6729559300637f; // 100.0f before

	/* Max world size = 255 by 255 by 255 (with top left corner of block as origin)
	So, max look distance in world = sqrt(255 * 255 + 255 * 255 + 255 * 255), which equals 441.6729559300637 */

	mat4 projection, view, model = GLM_MAT4_IDENTITY_INIT, view_times_model, model_view_projection;

	glm_perspective(to_radians(FOV), (GLfloat) SCR_W / SCR_H, near_clip_plane, far_clip_plane, projection);
	glm_lookat(pos, rel_origin, up, view);
	glm_mul(view, model, view_times_model);
	glm_mul(projection, view_times_model, model_view_projection);

	static GLuint matrix_id;
	static byte first_call = 1;

	if (first_call) {
		matrix_id = glGetUniformLocation(shader_program, "MVP");
		first_call = 0;
	}

	glUniformMatrix4fv(matrix_id, 1, GL_FALSE, &model_view_projection[0][0]);
}

void demo_2_matrix_setup(const GLuint shader_program, vec3 camera_pos) {
	vec3 origin = {0.0f, 0.0f, 0.0f}, up = {0.0f, 1.0f, 0.0f};
	demo_2_configurable_matrix_setup(shader_program, camera_pos, origin, up);
}

StateGL demo_2_init(void) {
	StateGL sgl;
	demo_1_init_vertex_data(&sgl);
	sgl.num_textures = 0;

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
