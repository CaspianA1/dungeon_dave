#include "rendering/shadow.h"
#include "utils/safe_io.h" // For `open_file_safely`
#include "utils/macro_utils.h" // For `ASSET_PATH`
#include "utils/alloc.h" // For `alloc`, and `dealloc`
#include "utils/texture.h" // For `init_texture_data`
#include "utils/opengl_wrappers.h" // For various OpenGL wrappers

/*
https://learnopengl.com/Guest-Articles/2021/CSM

For later on:
- Pushing the weapon against walls puts it in shadow, which doesn't look right (I need to keep it outside the wall)
- Figure out why for more cascades, there's more edge out-of-bounds artifacts

Revectorization:
- https://www.gamedev.net/tutorials/programming/graphics/shadow-map-silhouette-revectorization-smsr-r3437/
- https://graphicsinterface.org/wp-content/uploads/gi2016-10.pdf

Stabilization things:
https://lxjk.github.io/2017/04/15/Calculate-Minimal-Bounding-Sphere-of-Frustum.html
https://stackoverflow.com/questions/29557752/stable-shadow-mapping
https://web.archive.org/web/20210723225508/http://dev.theomader.com/stable-csm/
https://www.gamedev.net/forums/topic/673197-cascaded-shadow-map-shimmering-effect/5262338/
https://gamedev.stackexchange.com/questions/34782/shadows-shimmer-when-camera-moves
https://chetanjags.wordpress.com/2015/02/05/real-time-shadows-cascaded-shadow-maps/
http://www.diva-portal.org/smash/get/diva2:1056408/FULLTEXT01.pdf

https://community.khronos.org/t/how-can-i-fix-shadow-edge-swimming-in-csm/64997/2
https://ubm-twvideo01.s3.amazonaws.com/o1/vault/gdc09/slides/100_Handout%203.pdf
https://www.junkship.net/News/2020/11/22/shadow-of-a-doubt-part-2
https://github.com/TheRealMJP/Shadows/blob/master/Shadows/MeshRenderer.cpp
*/

static void get_light_view_projection(
	const CascadedShadowContext* const shadow_context,
	const mat4 camera_view, const vec3 dir_to_light,
	const GLfloat near_clip_dist, const GLfloat far_clip_dist,
	GLfloat aspect_ratio, mat4 light_view_projection) {

	////////// Getting the camera sub frustum center

	// The average FOV (in between the minimum and maximum) (TODO: perhaps use the full FOV, to avoid out-of-bounds areas)
	const GLfloat avg_fov = constants.camera.init_fov + constants.camera.limits.fov_change * 0.5f;

	mat4 camera_sub_frustum_projection, camera_sub_frustum_view_projection, inv_camera_sub_frustum_view_projection;
	glm_perspective(avg_fov, aspect_ratio, near_clip_dist, far_clip_dist, camera_sub_frustum_projection);
	glm_mul(camera_sub_frustum_projection, (vec4*) camera_view, camera_sub_frustum_view_projection);
	glm_mat4_inv(camera_sub_frustum_view_projection, inv_camera_sub_frustum_view_projection);

	vec4 camera_sub_frustum_corners[corners_per_frustum], camera_sub_frustum_center;
	glm_frustum_corners(inv_camera_sub_frustum_view_projection, camera_sub_frustum_corners);
	glm_frustum_center(camera_sub_frustum_corners, camera_sub_frustum_center);

	////////// Getting the light view

	vec3 light_eye;
	glm_vec3_add(camera_sub_frustum_center, (GLfloat*) dir_to_light, light_eye);

	mat4 light_view;
	glm_lookat(light_eye, camera_sub_frustum_center, GLM_YUP, light_view);

	////////// Finding the radius

	GLfloat radius_squared = -1.0f;

	for (byte i = 0; i < corners_per_frustum; i++) {
		const GLfloat dist = glm_vec3_distance2(camera_sub_frustum_corners[i], camera_sub_frustum_center);
		radius_squared = glm_max(radius_squared, dist);
	}

	const GLfloat radius = sqrtf(radius_squared) * shadow_context -> sub_frustum_scale;

	////////// Texel snapping

	const GLfloat divisor = 2.0f / shadow_context -> resolution * radius;
	GLfloat* const translation = light_view[3];

	translation[0] -= remainderf(translation[0], divisor);
	translation[1] -= remainderf(translation[1], divisor);

	////////// Making the projection and view projection matrices

	mat4 light_projection;
	glm_ortho(-radius, radius, -radius, radius, -radius, radius, light_projection);
	glm_mat4_mul(light_projection, light_view, light_view_projection);
}

//////////

/* For shaders, the number of cascades equals the value of the macro `NUM_CASCADES`
in `num_cascades.geom`. This is a macro, and not a uniform, since the shadow
geometry shader must clone the scene geometry a fixed number of times
(specified at compile time) for different layered rendering passes per each sub-frustum.
So, before all shader compilation, this function writes the number of cascades to `num_cascades.geom.` */
void specify_cascade_count_before_any_shader_compilation(
	const byte opengl_major_minor_version[2], const byte num_cascades) {

	FILE* const file = open_file_safely(ASSET_PATH("shaders/shadow/num_cascades.glsl"), "w");

	const GLchar* const file_description = "This file is written to before any other shaders include it";

	fprintf(file,
		"#version %hhu%hhu0 core\n\n// %s\n"
		"#define NUM_CASCADES %hhuu\n"
		"#define NUM_CASCADE_SPLITS %hhuu\n",

		opengl_major_minor_version[0],
		opengl_major_minor_version[1], file_description,
		num_cascades, (byte) (num_cascades - 1u));

	fclose(file);
}

CascadedShadowContext init_shadow_context(const CascadedShadowContextConfig* const config, const GLfloat far_clip_dist) {
	////////// Creating the split dists

	const byte num_cascades = config -> num_cascades;

	const byte num_split_dists = num_cascades - 1;
	GLfloat* const split_dists = alloc((size_t) num_split_dists, sizeof(GLfloat));
	mat4* const light_view_projection_matrices = alloc((size_t) num_cascades, sizeof(mat4));

	const GLfloat
		near_clip_dist = constants.camera.near_clip_dist,
		clip_dist_diff = far_clip_dist - constants.camera.near_clip_dist,
		linear_split_weight = config -> linear_split_weight;

	for (byte i = 0; i < num_split_dists; i++) {
		const GLfloat layer_percent = (GLfloat) (i + 1) / num_cascades;

		const GLfloat linear_dist = near_clip_dist + layer_percent * clip_dist_diff;
		const GLfloat log_dist = near_clip_dist * powf(far_clip_dist / near_clip_dist, layer_percent);

		split_dists[i] = glm_lerp(log_dist, linear_dist, linear_split_weight);
	}

	////////// Creating the depth layers

	GLuint depth_layers;
	glGenTextures(1, &depth_layers);
	use_texture(shadow_map_texture_type, depth_layers);

	GLint internal_format;

	const byte num_depth_buffer_bits = config -> num_depth_buffer_bits;

	switch (num_depth_buffer_bits) {
		#define INTERNAL_FORMAT_CASE(num_bits) case num_bits: internal_format = GL_DEPTH_COMPONENT##num_bits; break

		INTERNAL_FORMAT_CASE(16);
		INTERNAL_FORMAT_CASE(24);
		INTERNAL_FORMAT_CASE(32);

		#undef INTERNAL_FORMAT_CASE

		default: FAIL(CreateTexture, "Could not create a shadow map texture, because the number "
			"of depth buffer bits must be 16, 24, or 32, not %d", num_depth_buffer_bits);
	}

	const uint16_t resolution = config -> resolution;

	init_texture_data(shadow_map_texture_type, (GLsizei[]) {resolution, resolution, num_cascades},
		GL_DEPTH_COMPONENT, internal_format, OPENGL_COLOR_CHANNEL_TYPE, NULL);

	/* TODO: check that the number of cascades does not exceed the max size for `gl_Layer`
	(see the bottom of https://www.khronos.org/opengl/wiki/Geometry_Shader for more info),
	and check for a minimum split count too */

	////////// Creating the framebuffer

	GLuint framebuffer = init_framebuffer();
	use_framebuffer(framebuffer_target, framebuffer);

	glFramebufferTexture(framebuffer_target, GL_DEPTH_ATTACHMENT, depth_layers, 0);
	glDrawBuffer(GL_NONE); glReadBuffer(GL_NONE); // Not drawing into or reading from any color buffers
	check_framebuffer_completeness();

	use_framebuffer(framebuffer_target, 0);

	////////// Creating the depth samplers

	GLuint depth_samplers[2];
	glGenSamplers(2, depth_samplers);

	const GLuint plain_depth_sampler = depth_samplers[0];
	glSamplerParameteri(plain_depth_sampler, GL_TEXTURE_WRAP_S, TexNonRepeating);
	glSamplerParameteri(plain_depth_sampler, GL_TEXTURE_WRAP_T, TexNonRepeating);
	glSamplerParameteri(plain_depth_sampler, GL_TEXTURE_MAG_FILTER, OPENGL_SCENE_MAG_FILTER);
	glSamplerParameteri(plain_depth_sampler, GL_TEXTURE_MIN_FILTER, OPENGL_SCENE_MAG_FILTER);

	const GLuint depth_comparison_sampler = depth_samplers[1];
	glSamplerParameteri(depth_comparison_sampler, GL_TEXTURE_WRAP_S, TexNonRepeating);
	glSamplerParameteri(depth_comparison_sampler, GL_TEXTURE_WRAP_T, TexNonRepeating);
	glSamplerParameteri(depth_comparison_sampler, GL_TEXTURE_MAG_FILTER, OPENGL_SCENE_MAG_FILTER);
	glSamplerParameteri(depth_comparison_sampler, GL_TEXTURE_MIN_FILTER, OPENGL_SCENE_MAG_FILTER);
	glSamplerParameteri(depth_comparison_sampler, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glSamplerParameteri(depth_comparison_sampler, GL_TEXTURE_COMPARE_FUNC, GL_LESS);

	//////////

	return (CascadedShadowContext) {
		.framebuffer = framebuffer,
		.depth_layers = depth_layers,
		.plain_depth_sampler = plain_depth_sampler,
		.depth_comparison_sampler = depth_comparison_sampler,

		.resolution = resolution, .num_cascades = num_cascades,
		.sub_frustum_scale = config -> sub_frustum_scale,

		.split_dists = split_dists,
		.light_view_projection_matrices = light_view_projection_matrices
	};
}

void deinit_shadow_context(const CascadedShadowContext* const shadow_context) {
	dealloc(shadow_context -> split_dists);
	dealloc(shadow_context -> light_view_projection_matrices);
	deinit_texture(shadow_context -> depth_layers);
	deinit_framebuffer(shadow_context -> framebuffer);
	glDeleteSamplers(2, (GLuint[]) {shadow_context -> plain_depth_sampler, shadow_context -> depth_comparison_sampler});
}

void update_shadow_context(const CascadedShadowContext* const shadow_context,
	const Camera* const camera, const vec3 dir_to_light, const GLfloat aspect_ratio) {

	const GLfloat* const split_dists = shadow_context -> split_dists;
	mat4* const light_view_projection_matrices = shadow_context -> light_view_projection_matrices;
	const vec4* const camera_view = camera -> view;

	const GLsizei num_cascades = shadow_context -> num_cascades;
	const GLfloat far_clip_dist = camera -> far_clip_dist;

	////////// Getting the matrices needed

	for (GLsizei i = 0; i < num_cascades; i++) {
		GLfloat sub_near_clip, sub_far_clip;

		if (i == 0) {
			sub_near_clip = constants.camera.near_clip_dist;
			sub_far_clip = split_dists[0];
		}
		else {
			sub_near_clip = split_dists[i - 1];
			sub_far_clip = (i == num_cascades - 1) ? far_clip_dist : split_dists[i];
		}

		get_light_view_projection(shadow_context, camera_view, dir_to_light, sub_near_clip,
			sub_far_clip, aspect_ratio, light_view_projection_matrices[i]);
	}
}

void enable_rendering_to_shadow_context(const CascadedShadowContext* const shadow_context) {
	const uint16_t resolution = shadow_context -> resolution;

	glViewport(0, 0, resolution, resolution);
	use_framebuffer(framebuffer_target, shadow_context -> framebuffer);
	glClear(GL_DEPTH_BUFFER_BIT);
}

void disable_rendering_to_shadow_context(const GLint screen_size[2]) {
	use_framebuffer(framebuffer_target, 0);
	glViewport(0, 0, screen_size[0], screen_size[1]);
}
