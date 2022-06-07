#ifndef CSM_C
#define CSM_C

#include "headers/csm.h"
#include "headers/constants.h"
#include "headers/shader.h"
#include "headers/texture.h"

#define CSM_SIZED_DEPTH_FORMAT GL_DEPTH_COMPONENT16

/*
https://learnopengl.com/Guest-Articles/2021/CSM

For later on:
- Texel snapping
- Blending between layers
- Filtering
- Merging the master branch with this one
*/

//////////

static void get_csm_light_view_projection_matrix(const Camera* const camera,
	const GLfloat near_clip, const GLfloat far_clip, const GLfloat z_scale,
	const vec3 light_dir, mat4 light_view_projection) {

	////////// Getting the sub frustum center

	mat4 camera_sub_frustum_projection, camera_sub_frustum_view_projection, inv_camera_sub_frustum_view_projection;

	glm_perspective(camera -> angles.fov, camera -> aspect_ratio,
		near_clip, far_clip, camera_sub_frustum_projection);

	glm_mul(camera_sub_frustum_projection, (vec4*) camera -> view, camera_sub_frustum_view_projection);
	glm_mat4_inv(camera_sub_frustum_view_projection, inv_camera_sub_frustum_view_projection);

	vec4 camera_sub_frustum_corners[8], camera_sub_frustum_center;
	glm_frustum_corners(inv_camera_sub_frustum_view_projection, camera_sub_frustum_corners);
	glm_frustum_center(camera_sub_frustum_corners, camera_sub_frustum_center);

	////////// Getting the light view

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
	const GLfloat far_clip_dist, const GLfloat linear_split_weight, const GLsizei width,
	const GLsizei height, const GLsizei num_layers) {

	const GLuint depth_layers = init_csm_depth_layers(width, height, num_layers);

	//////////

	List
		split_dists = init_list((buffer_size_t) num_layers - 1, GLfloat),
		light_view_projection_matrices = init_list((buffer_size_t) num_layers, mat4);

	light_view_projection_matrices.length = light_view_projection_matrices.max_alloc;
	split_dists.length = split_dists.max_alloc;

	const GLfloat
		near_dist = constants.camera.near_clip_dist,
		clip_dist_diff = far_clip_dist - constants.camera.near_clip_dist;

	for (buffer_size_t i = 0; i < split_dists.length; i++) {
		const GLfloat layer_percent = (GLfloat) (i + 1) / num_layers;

		const GLfloat linear_dist = near_dist + layer_percent * clip_dist_diff;
		const GLfloat log_dist = near_dist * powf(far_clip_dist / near_dist, layer_percent);

		const GLfloat weighted_dist = glm_lerp(log_dist, linear_dist, linear_split_weight);

		*((GLfloat*) ptr_to_list_index(&split_dists, i)) = weighted_dist;
	}

	return (CascadedShadowContext) {
		.depth_layers = depth_layers,
		.framebuffer = init_csm_framebuffer(depth_layers),

		.depth_shader = init_shader("assets/shaders/csm/depth.vert",
			"assets/shaders/csm/depth.geom", "assets/shaders/csm/depth.frag"),

		.resolution = {width, height},

		.z_scale = z_scale,
		.light_dir = {light_dir[0], light_dir[1], light_dir[2]},
		.split_dists = split_dists,
		.light_view_projection_matrices = light_view_projection_matrices
	};
}

void deinit_csm_context(const CascadedShadowContext* const csm_context) {
	deinit_list(csm_context -> split_dists);
	deinit_list(csm_context -> light_view_projection_matrices);
	deinit_shader(csm_context -> depth_shader);
	deinit_texture(csm_context -> depth_layers);
	glDeleteFramebuffers(1, &csm_context -> framebuffer);
}

void draw_to_csm_context(const CascadedShadowContext* const csm_context, const Camera* const camera,
	const GLint screen_size[2], void (*const drawer) (const void* const), const void* const drawer_param) {

	const List
		*const light_view_projection_matrices = &csm_context -> light_view_projection_matrices,
		*const split_dists = &csm_context -> split_dists;

	const buffer_size_t num_cascades = light_view_projection_matrices -> length;

	const GLfloat
		z_scale = csm_context -> z_scale,
		far_clip_dist = camera -> far_clip_dist,
		*const light_dir = csm_context -> light_dir;

	////////// Getting the matrices needed

	for (buffer_size_t i = 0; i < num_cascades; i++) {
		GLfloat near_clip, far_clip;

		if (i == 0) {
			near_clip = constants.camera.near_clip_dist;
			far_clip = value_at_list_index(split_dists, i, GLfloat);
		}
		else {
			near_clip = value_at_list_index(split_dists, i - 1, GLfloat);
			far_clip = (i == num_cascades - 1) ? far_clip_dist : value_at_list_index(split_dists, i, GLfloat);
		}

		vec4* const matrix = ptr_to_list_index(light_view_projection_matrices, i);
		get_csm_light_view_projection_matrix(camera, near_clip, far_clip, z_scale, light_dir, matrix);
	}

	////////// Updating the light view projection matrices uniform

	const GLuint depth_shader = csm_context -> depth_shader;
	use_shader(depth_shader);

	static GLint light_view_projection_matrices_id;
	ON_FIRST_CALL(INIT_UNIFORM(light_view_projection_matrices, depth_shader););

	UPDATE_UNIFORM(light_view_projection_matrices, Matrix4fv, (GLsizei)
		num_cascades, GL_FALSE, light_view_projection_matrices -> data);

	////////// Rendering to the cascades

	const GLsizei* const resolution = csm_context -> resolution;
	glViewport(0, 0, resolution[0], resolution[1]);
	glBindFramebuffer(GL_FRAMEBUFFER, csm_context -> framebuffer);
	glCullFace(GL_FRONT);

	glClear(GL_DEPTH_BUFFER_BIT);
	drawer(drawer_param);

	glCullFace(GL_BACK);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, screen_size[0], screen_size[1]);
}

#endif
