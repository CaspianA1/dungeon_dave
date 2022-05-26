#ifndef CSM_C
#define CSM_C

#include "headers/csm.h"

// https://learnopengl.com/Guest-Articles/2021/CSM

/*
Details:
- Have the MVP getter set up
- Will use array textures at first for layers (with the normal framebuffer setup)

- Geo shader setup:
- Vertex shader: normal gl_Position setting
- Fragment shader: some special code
- Then, the remaining code concerns cascade selection
*/

//////////

static void init_csm_geo_shader(void) {
	const GLchar* const geo_shader_code =
		// Hm, this does not work under 3.3

		"#version 330 core\n"

		"layout(triangles, invocations = 5) in;\n"
		"layout(triangle_strip, max_vertices = 3) out;\n"

		"uniform mat4 light_space_matrices[16];\n"

		"void main(void) {\n"
			"for (int i = 0; i < 3; i++) {\n"
				"gl_Position = light_space_matrices[gl_InvocationID] * gl_in[i].gl_Position;\n"
				"gl_Layer = gl_InvocationID;\n"
				"EmitVertex();\n"
			"}\n"
		"EndPrimitive();\n"
	"}\n";

	//////////

	puts("Initing the csm geo shader");

	const GLuint shader = glCreateProgram();

	const GLuint geo_sub_shader = glCreateShader(GL_GEOMETRY_SHADER);
	glShaderSource(geo_sub_shader, (GLsizei) 1, &geo_shader_code, NULL);
	glCompileShader(geo_sub_shader);

	GLint log_length;
	glGetShaderiv(geo_sub_shader, GL_INFO_LOG_LENGTH, &log_length);

	if (log_length > 0) {
		GLchar* const error_message = malloc((size_t) log_length + 1);
		glGetShaderInfoLog(geo_sub_shader, log_length, NULL, error_message);
		DEBUG(error_message, s);
		free(error_message);

		GL_ERR_CHECK;
		puts("Problemo");
		return;
	}

	puts("All bueno");

	glAttachShader(shader, geo_sub_shader);

	/*
	- Now, will also need a vertex and fragment shader.
	- Would be so much easier to be able to inject in a geo shader.
	- TODO: add that functionality.
	*/
}

// TODO: split up this function wherever possible
static void get_csm_model_view_projection(const Camera* const camera,
	const vec3 light_dir, const GLfloat z_scale, mat4 model_view_projection) {

	enum {corners_per_frustum = 8};

	////////// Inv model view projection -> frustum corners -> frustum center -> light eye -> view

	mat4 inv_model_view_projection;
	glm_mat4_inv((vec4*) camera -> model_view_projection, inv_model_view_projection);

	vec4 frustum_corners[corners_per_frustum];
	glm_frustum_corners(inv_model_view_projection, frustum_corners);

	vec4 frustum_center;
	glm_frustum_center(frustum_corners, frustum_center);

	vec3 light_eye;
	glm_vec3_add(frustum_center, (GLfloat*) light_dir, light_eye);

	mat4 view;
	glm_lookat(light_eye, frustum_center, (vec3) {0.0f, 1.0f, 0.0f}, view);

	////////// Frustum box -> scaling min and max z -> projection

	vec3 frustum_box[2];
	glm_frustum_box(frustum_corners, view, frustum_box);

	GLfloat min_z = frustum_box[0][2];
	frustum_box[0][2] = (min_z < 0.0f) ? (min_z * z_scale) : (min_z / z_scale);

	GLfloat max_z = frustum_box[1][2];
	frustum_box[1][2] = (max_z < 0.0f) ? (max_z / z_scale) : (max_z * z_scale);

	mat4 projection;
	glm_ortho_aabb(frustum_box, projection);
	glm_mul(projection, view, model_view_projection);
}

void csm_test(const Camera* const camera, const vec3 light_dir) {
	// ON_FIRST_CALL(init_csm_geo_shader(););

	mat4 model_view_projection;
	get_csm_model_view_projection(camera, light_dir, 10.0f, model_view_projection);

}

#endif
