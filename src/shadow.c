#ifndef SHADOW_C
#define SHADOW_C

#include "headers/shadow.h"
#include "headers/constants.h"
#include "headers/shader.h"
#include "headers/texture.h"

/*
https://learnopengl.com/Guest-Articles/2021/CSM

For later on:
- Texel snapping through reprojection for more preserved resolution and less jittering artifacts
- Merging the master branch with this one
- A world-space approach to merging the AABB of the sub frustum box
	with PSRs, instead of defining a scale factor for the frustum
- Pushing the weapon against walls puts it in shadow, which doesn't look right (I need to keep it outside the wall)
- Try to avoid the use of a geometry shader for instancing if possible
	(can probably offset vertices with normal instancing, but I don't know about changing `gl_Layer`)

Revectorization:
- https://www.gamedev.net/tutorials/programming/graphics/shadow-map-silhouette-revectorization-smsr-r3437/
- https://graphicsinterface.org/wp-content/uploads/gi2016-10.pdf
*/

////////// This part concerns getting the light view projection matrix of a camera sub frustum

static void get_camera_sub_frustum_corners(const Camera* const camera, const GLfloat near_clip_dist,
	const GLfloat far_clip_dist, vec4 camera_sub_frustum_corners[corners_per_frustum]) {

	mat4 camera_sub_frustum_projection, camera_sub_frustum_view_projection, inv_camera_sub_frustum_view_projection;

	glm_perspective(camera -> angles.fov, camera -> aspect_ratio,
		near_clip_dist, far_clip_dist, camera_sub_frustum_projection);

	glm_mul(camera_sub_frustum_projection, (vec4*) camera -> view, camera_sub_frustum_view_projection);
	glm_mat4_inv(camera_sub_frustum_view_projection, inv_camera_sub_frustum_view_projection);
	glm_frustum_corners(inv_camera_sub_frustum_view_projection, camera_sub_frustum_corners);
}

static void get_light_view_and_projection(const CascadedShadowContext* const shadow_context,
	const vec4 camera_sub_frustum_corners[corners_per_frustum], mat4 light_view, mat4 light_projection) {

	////////// Getting the sub frustum center from the sub frustum corners, and getting a light view from that

	vec3 light_eye;
	vec4 camera_sub_frustum_center;

	glm_frustum_center((vec4*) camera_sub_frustum_corners, camera_sub_frustum_center);
	glm_vec3_add(camera_sub_frustum_center, (GLfloat*) shadow_context -> dir_to_light, light_eye);
	glm_lookat(light_eye, camera_sub_frustum_center, GLM_YUP, light_view);

	////////// Getting an AABB from the sub frustum corners, and scaling it up

	vec3 light_view_sub_frustum_box[2];
	glm_frustum_box((vec4*) camera_sub_frustum_corners, (vec4*) light_view, light_view_sub_frustum_box);

	const GLfloat* const light_view_sub_frustum_box_scale = shadow_context -> sub_frustum_scale;

	for (byte i = 0; i < 3; i++) {
		const GLfloat scale = light_view_sub_frustum_box_scale[i];
		const GLfloat one_over_scale = 1.0f / scale;

		GLfloat
			*const min = &light_view_sub_frustum_box[0][i],
			*const max = &light_view_sub_frustum_box[1][i];

		*min *= (*min < 0.0f) ? scale : one_over_scale;
		*max *= (*max < 0.0f) ? one_over_scale : scale;
	}

	////////// Getting the AABB radius, and applying texel snapping to the light view based on that

	const GLfloat radius = roundf(glm_aabb_radius(light_view_sub_frustum_box));
	const GLfloat divisor = 2.0f * radius / shadow_context -> resolution;

	GLfloat* const column = light_view[3];
	for (byte i = 0; i < 3; i++) column[i] -= fmodf(column[i], divisor);

	////////// Getting the light projection

	glm_ortho(-radius, radius, -radius, radius, -radius, radius, light_projection);

}

static void get_sub_frustum_light_view_projection_matrix(const Camera* const camera,
	const CascadedShadowContext* const shadow_context, const GLfloat near_clip_dist,
	const GLfloat far_clip_dist, mat4 light_view_projection) {

	vec4 camera_sub_frustum_corners[corners_per_frustum];
	get_camera_sub_frustum_corners(camera, near_clip_dist, far_clip_dist, camera_sub_frustum_corners);

	mat4 light_view, light_projection;
	get_light_view_and_projection(shadow_context, camera_sub_frustum_corners, light_view, light_projection);
	glm_mul(light_projection, light_view, light_view_projection);
}

//////////

static GLuint init_csm_depth_layers(const GLsizei resolution, const GLsizei num_cascades) {
	const GLuint depth_layers = preinit_texture(TexSet, TexNonRepeating, OPENGL_SHADOW_MAP_MAG_FILTER, OPENGL_SHADOW_MAP_MIN_FILTER, true);
	glTexImage3D(TexSet, 0, OPENGL_SIZED_SHADOW_MAP_PIXEL_FORMAT, resolution, resolution, num_cascades, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	return depth_layers;
}

static GLuint init_csm_framebuffer(const GLuint depth_layers) {
	GLuint framebuffer;

	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_layers, 0);
	glDrawBuffer(GL_NONE); // Not drawing into any color buffers
	glReadBuffer(GL_NONE); // Not reading from any color buffers

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		FAIL(CreateFramebuffer, "OpenGL error is '%s'", get_GL_error());

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return framebuffer;
}

/* For shaders, the number of cascades equals the value of the macro `NUM_CASCADES`
in `num_cascades.geom`. This is a macro, and not a uniform, since the shadow
geometry shader must clone the scene geometry a fixed number of times
(specified at compile time). `cascade_split_distances` has a compile-time size
as well. So, before all shader compilation, this function writes the number of
cascades to `num_cascades.geom.` */
void specify_cascade_count_before_any_shader_compilation(const GLsizei num_cascades) {
	FILE* const file = open_file_safely(ASSET_PATH("shaders/common/shadow/num_cascades.glsl"), "w");

	const byte* const opengl_version = constants.window.opengl_major_minor_version;
	const GLchar* const file_description = "This file is written to before any other shaders include it";

	fprintf(file, "#version %u%u0 core\n\n// %s\n#define NUM_CASCADES %uu\n",
		opengl_version[0], opengl_version[1], file_description, num_cascades);

	fclose(file);
}

CascadedShadowContext init_shadow_context(
	const vec3 dir_to_light, const vec3 sub_frustum_scale,
	const GLfloat far_clip_dist, const GLfloat linear_split_weight,
	const GLsizei resolution, const GLsizei num_cascades) {

	const GLsizei num_split_dists = num_cascades - 1;
	GLfloat* const split_dists = malloc((size_t) num_split_dists * sizeof(GLfloat));
	mat4* const light_view_projection_matrices = malloc((size_t) num_cascades * sizeof(mat4));

	const GLfloat
		near_clip_dist = constants.camera.near_clip_dist,
		clip_dist_diff = far_clip_dist - constants.camera.near_clip_dist;

	//////////

	for (GLsizei i = 0; i < num_split_dists; i++) {
		const GLfloat layer_percent = (GLfloat) (i + 1) / num_cascades;

		const GLfloat linear_dist = near_clip_dist + layer_percent * clip_dist_diff;
		const GLfloat log_dist = near_clip_dist * powf(far_clip_dist / near_clip_dist, layer_percent);

		split_dists[i] = glm_lerp(log_dist, linear_dist, linear_split_weight);
	}

	//////////

	const GLuint depth_layers = init_csm_depth_layers(resolution, num_cascades);

	return (CascadedShadowContext) {
		.depth_layers = depth_layers,
		.framebuffer = init_csm_framebuffer(depth_layers),

		.depth_shader = init_shader(ASSET_PATH("shaders/common/shadow/depth.vert"),
			ASSET_PATH("shaders/common/shadow/depth.geom"), ASSET_PATH("shaders/common/shadow/depth.frag")
		),

		.resolution = resolution,

		.dir_to_light = {dir_to_light[0], dir_to_light[1], dir_to_light[2]},
		.sub_frustum_scale = {sub_frustum_scale[0], sub_frustum_scale[1], sub_frustum_scale[2]},

		.num_cascades = num_cascades, .split_dists = split_dists,
		.light_view_projection_matrices = light_view_projection_matrices
	};
}

void deinit_shadow_context(const CascadedShadowContext* const shadow_context) {
	free(shadow_context -> split_dists);
	free(shadow_context -> light_view_projection_matrices);
	deinit_shader(shadow_context -> depth_shader);
	deinit_texture(shadow_context -> depth_layers);
	glDeleteFramebuffers(1, &shadow_context -> framebuffer);
}

void enable_rendering_to_shadow_context(const CascadedShadowContext* const shadow_context, const Camera* const camera) {
	const GLsizei num_cascades = shadow_context -> num_cascades;
	const GLfloat* const split_dists = shadow_context -> split_dists;
	mat4* const light_view_projection_matrices = shadow_context -> light_view_projection_matrices;

	const GLfloat far_clip_dist = camera -> far_clip_dist;

	////////// Getting the matrices needed

	for (GLsizei i = 0; i < num_cascades; i++) {
		GLfloat sub_near_clip, sub_far_clip;

		if (i == 0) {
			sub_near_clip = constants.camera.near_clip_dist;
			sub_far_clip = split_dists[i];
		}
		else {
			sub_near_clip = split_dists[i - 1];
			sub_far_clip = (i == num_cascades - 1) ? far_clip_dist : split_dists[i];
		}

		get_sub_frustum_light_view_projection_matrix(camera, shadow_context,
			sub_near_clip, sub_far_clip, light_view_projection_matrices[i]);
	}

	////////// Updating the light view projection matrices uniform

	const GLuint depth_shader = shadow_context -> depth_shader;
	use_shader(depth_shader);

	static GLint light_view_projection_matrices_id;
	ON_FIRST_CALL(INIT_UNIFORM(light_view_projection_matrices, depth_shader););

	UPDATE_UNIFORM(light_view_projection_matrices, Matrix4fv, (GLsizei) num_cascades, GL_FALSE, (GLfloat*) light_view_projection_matrices);

	////////// Preparing for cascade rendering

	const GLsizei resolution = shadow_context -> resolution;
	glViewport(0, 0, resolution, resolution);
	glBindFramebuffer(GL_FRAMEBUFFER, shadow_context -> framebuffer);

	glClear(GL_DEPTH_BUFFER_BIT);
}

void disable_rendering_to_shadow_context(const GLint screen_size[2]) {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, screen_size[0], screen_size[1]);
}

#endif
