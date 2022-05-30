#ifndef CSM_C
#define CSM_C

#include "headers/csm.h"
#include "headers/shader.h"
#include "headers/texture.h"

#define CSM_SIZED_DEPTH_FORMAT GL_DEPTH_COMPONENT16

// https://learnopengl.com/Guest-Articles/2021/CSM

/*
Details:
- Have the MVP getter set up
- Will use array textures at first for layers (with the normal framebuffer setup)
- The remaining code concerns cascade selection in the depth + other shader
*/

//////////

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

	GLfloat* const min_z_ref = &frustum_box[0][2];
	GLfloat min_z = *min_z_ref;
	*min_z_ref = (min_z < 0.0f) ? (min_z * z_scale) : (min_z / z_scale);

	GLfloat* const max_z_ref = &frustum_box[1][2];
	GLfloat max_z = *max_z_ref;
	*max_z_ref = (max_z < 0.0f) ? (max_z / z_scale) : (max_z * z_scale);

	mat4 projection;
	glm_ortho_aabb(frustum_box, projection);
	glm_mul(projection, view, model_view_projection);
}

static GLuint init_csm_depth_layers(const GLsizei width,
	const GLsizei height, const GLsizei num_layers) {

	GLuint depth_layers;
	glGenTextures(1, &depth_layers);
	set_current_texture(TexSet, depth_layers);

	glTexParameteri(TexSet, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(TexSet, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(TexSet, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(TexSet, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage3D(TexSet, 0, CSM_SIZED_DEPTH_FORMAT, width, height, num_layers, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	return depth_layers;
}

static GLuint init_csm_framebuffer(const GLuint depth_layers) {
	GLuint framebuffer;

	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_layers, 0);
	glDrawBuffer(GL_NONE);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		FAIL(CreateFramebuffer, "OpenGL error is '%s'", get_GL_error());

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return framebuffer;
}

CascadedShadowContext init_csm_context(const vec3 light_dir, const GLfloat z_scale,
	const GLsizei width, const GLsizei height, const GLsizei num_layers) {

	const GLuint depth_layers = init_csm_depth_layers(width, height, num_layers);

	return (CascadedShadowContext) {
		.depth_layers = depth_layers,
		.framebuffer = init_csm_framebuffer(depth_layers),

		.depth_shader = init_shader("assets/shaders/csm/depth.vert",
			"assets/shaders/csm/depth.geom", "assets/shaders/csm/depth.frag"),

		.z_scale = z_scale,
		.light_dir = {light_dir[0], light_dir[1], light_dir[2]}
	};
}

void deinit_csm_context(const CascadedShadowContext* const csm_context) {
	deinit_shader(csm_context -> depth_shader);
	deinit_texture(csm_context -> depth_layers);
	glDeleteFramebuffers(1, &csm_context -> framebuffer);
}

void render_to_csm_context(const CascadedShadowContext* const csm_context, const Camera* const camera) {
	mat4 model_view_projection;
	get_csm_model_view_projection(camera, csm_context -> light_dir, csm_context -> z_scale, model_view_projection);
	// TODO: actually do stuff here
}

#endif
