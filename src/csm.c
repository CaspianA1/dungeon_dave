#ifndef CSM_C
#define CSM_C

#include "headers/csm.h"
#include "headers/shader.h"
#include "headers/texture.h"

// https://learnopengl.com/Guest-Articles/2021/CSM

/*
Details:
- Have the MVP getter set up
- Will use array textures at first for layers (with the normal framebuffer setup)

- The remaining code concerns cascade selection in the depth + other shader
*/

//////////

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

static void init_csm_layered_texture(const GLsizei width, const GLsizei height, const GLsizei num_layers, const GLint depth_format) {
	GLuint depth_layers;
	glGenTextures(1, &depth_layers);
	glBindTexture(TexSet, depth_layers);

	glTexParameteri(TexSet, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(TexSet, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(TexSet, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(TexSet, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage3D(TexSet, 0, depth_format, width, height, num_layers, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	GLuint framebuffer;
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_layers, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		FAIL(CreateFramebuffer, "OpenGL error is '%s'", get_GL_error());

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//////////

	glDeleteTextures(1, &depth_layers);
	glDeleteFramebuffers(1, &framebuffer);
}

void csm_test(const Camera* const camera, const vec3 light_dir) {
	// NEXT: set up a context similar to esm, where stuff can be drawn to the layers

	ON_FIRST_CALL(
		const GLuint shader = init_shader("assets/shaders/csm/depth.vert",
			"assets/shaders/csm/depth.geom", "assets/shaders/csm/depth.frag");

		deinit_shader(shader);

		init_csm_layered_texture(1024, 1024, 3, GL_DEPTH_COMPONENT16);
	);

	mat4 model_view_projection;
	get_csm_model_view_projection(camera, light_dir, 10.0f, model_view_projection);

}

#endif
