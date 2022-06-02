#ifndef CSM_C
#define CSM_C

#include "headers/csm.h"
#include "headers/constants.h"
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

static void get_csm_light_view_projection(const Camera* const camera,
	const GLfloat near_clip, const GLfloat far_clip, const GLfloat z_scale,
	const vec3 light_dir, mat4 light_view_projection) {

	////////// Getting sub frustum center

	mat4 camera_sub_frustum_projection, camera_sub_frustum_view_projection;

	glm_perspective(camera -> angles.fov, camera -> aspect_ratio,
		near_clip, far_clip, camera_sub_frustum_projection);

	glm_mul(camera_sub_frustum_projection, (vec4*) camera -> view, camera_sub_frustum_view_projection);

	vec4 camera_sub_frustum_corners[8], camera_sub_frustum_center;

	glm_frustum_corners(camera_sub_frustum_view_projection, camera_sub_frustum_corners);
	glm_frustum_center(camera_sub_frustum_corners, camera_sub_frustum_center);

	////////// Getting light view

	vec3 light_eye;
	glm_vec3_add(camera_sub_frustum_center, (GLfloat*) light_dir, light_eye);

	mat4 light_view;
	glm_lookat(light_eye, camera_sub_frustum_center, (vec3) {0.0f, 1.0f, 0.0f}, light_view);

	////////// Getting a bounding box of the light view

	vec3 light_view_frustum_box[2];
	glm_frustum_box(camera_sub_frustum_corners, light_view, light_view_frustum_box);

	GLfloat* const min_z_ref = &light_view_frustum_box[0][2];
	GLfloat min_z = *min_z_ref;
	*min_z_ref = (min_z < 0.0f) ? (min_z * z_scale) : (min_z / z_scale);

	GLfloat* const max_z_ref = &light_view_frustum_box[1][2];
	GLfloat max_z = *max_z_ref;
	*max_z_ref = (max_z < 0.0f) ? (max_z / z_scale) : (max_z * z_scale);

	////////// Using the light view frustum box, light projection, and light view to make a light view projection

	mat4 light_projection;
	glm_ortho_aabb(light_view_frustum_box, light_projection);
	glm_mul(light_projection, light_view, light_view_projection);
}

static GLuint init_csm_depth_layers(const GLsizei width,
	const GLsizei height, const GLsizei num_layers) {

	const GLuint depth_layers = preinit_texture(TexSet, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
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

	const GLfloat sub_near = 0.01f, sub_far = 5.0f;

	mat4 light_view_projection;
	get_csm_light_view_projection(camera, sub_near, sub_far, csm_context -> z_scale, csm_context -> light_dir, light_view_projection);

	/*
	for (byte y = 0; y < 4; y++) {
		for (byte x = 0; x < 4; x++) {
			printf("%lf ", light_view_projection[y][x]);
		}
	}
	puts("---");
	*/

	// TODO: actually do stuff here
}

#endif
