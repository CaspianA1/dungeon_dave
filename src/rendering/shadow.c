#include "rendering/shadow.h"
#include "utils/safe_io.h" // For `open_file_safely`
#include "utils/macro_utils.h" // For `ASSET_PATH`
#include "utils/alloc.h" // For `alloc`, and `dealloc`
#include "utils/texture.h" // For `init_texture_data`
#include "utils/opengl_wrappers.h" // For various OpenGL wrappers

static const GLenum framebuffer_target = GL_DRAW_FRAMEBUFFER;

/*
https://learnopengl.com/Guest-Articles/2021/CSM

For later on:
- A constant projection radius, for no jitter when increasing or decreasing the FOV
- Pushing the weapon against walls puts it in shadow, which doesn't look right (I need to keep it outside the wall)
- Try to avoid the use of a geometry shader for instancing if possible
	(can probably offset vertices with normal instancing, but I don't know about changing `gl_Layer`)
- Blend between layers in some way using hardware? Volume textures aren't usable for this, see
	the bottom of https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage3D.xhtml.
- Sometimes, shadows disappear from the view frustum when the shouldn't (the min of the 2 blended shadow values,
	or some modification of that?) (Or maybe because it's because the shadow frustum is fit around a too big FOV?)

Revectorization:
- https://www.gamedev.net/tutorials/programming/graphics/shadow-map-silhouette-revectorization-smsr-r3437/
- https://graphicsinterface.org/wp-content/uploads/gi2016-10.pdf

Stabilization things:
https://lxjk.github.io/2017/04/15/Calculate-Minimal-Bounding-Sphere-of-Frustum.html
https://stackoverflow.com/questions/29557752/stable-shadow-mapping
https://web.archive.org/web/20210723225508/http://dev.theomader.com/stable-csm/
https://www.junkship.net/News/2020/11/22/shadow-of-a-doubt-part-2
https://www.gamedev.net/forums/topic/673197-cascaded-shadow-map-shimmering-effect/5262338/
https://gamedev.stackexchange.com/questions/34782/shadows-shimmer-when-camera-moves
https://chetanjags.wordpress.com/2015/02/05/real-time-shadows-cascaded-shadow-maps/
http://www.diva-portal.org/smash/get/diva2:1056408/FULLTEXT01.pdf
*/

static void get_light_view_projection(
	const CascadedShadowContext* const shadow_context,
	const Camera* const camera, const vec3 dir_to_light,
	const GLfloat near_clip_dist, const GLfloat far_clip_dist,
	const GLfloat aspect_ratio, mat4 light_view_projection) {

	////////// Getting the camera sub frustum center

	mat4 camera_sub_frustum_projection, camera_sub_frustum_view_projection, inv_camera_sub_frustum_view_projection;
	glm_perspective(camera -> fov, aspect_ratio, near_clip_dist, far_clip_dist, camera_sub_frustum_projection);
	glm_mul(camera_sub_frustum_projection, (vec4*) camera -> view, camera_sub_frustum_view_projection);
	glm_mat4_inv(camera_sub_frustum_view_projection, inv_camera_sub_frustum_view_projection);

	vec4 camera_sub_frustum_corners[corners_per_frustum], camera_sub_frustum_center;
	glm_frustum_corners(inv_camera_sub_frustum_view_projection, camera_sub_frustum_corners);
	glm_frustum_center(camera_sub_frustum_corners, camera_sub_frustum_center);

	////////// Getting the light view

	vec3 light_eye;
	glm_vec3_add(camera_sub_frustum_center, (GLfloat*) dir_to_light, light_eye);

	mat4 light_view;
	glm_lookat(light_eye, camera_sub_frustum_center, GLM_YUP, light_view);

	////////// Texel snapping (used https://lxjk.github.io/2017/04/15/Calculate-Minimal-Bounding-Sphere-of-Frustum.html)

	/* When the camera FOV changes, each sub frustum gets wider, and this results in texel snapping not working.
	So, the FOV for each sub frustum is the average of the initial camera FOV and the max FOV, in order to best
	accomodate both FOVs. */
	const GLfloat fov_for_sub_frustum = constants.camera.init_fov + constants.camera.limits.fov_change * 0.5f;

	const GLfloat // TODO: precompute k
		far_clip_minus_near_clip = far_clip_dist - near_clip_dist,
		far_clip_plus_near_clip = far_clip_dist + near_clip_dist,
		k = sqrtf(1.0f + powf(1.0f / aspect_ratio, 2)) * tanf(fov_for_sub_frustum * 0.5f);

	const GLfloat k_squared = k * k;

	const GLfloat radius = shadow_context -> sub_frustum_scale * 0.5f * sqrtf(
		far_clip_minus_near_clip * far_clip_minus_near_clip
		+
		2.0f * (far_clip_dist * far_clip_dist + near_clip_dist * near_clip_dist) * k_squared
		+
		(far_clip_plus_near_clip * far_clip_plus_near_clip) * (k_squared * k_squared)
	);

	//////////

	const GLfloat divisor = 2.0f * radius / shadow_context -> resolution;

	GLfloat* const projected_center = light_view[3];
	projected_center[0] -= remainderf(projected_center[0], divisor);
	projected_center[1] -= remainderf(projected_center[1], divisor);

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
		"#define NUM_CASCADE_SPLITS %uu\n",

		opengl_major_minor_version[0],
		opengl_major_minor_version[1], file_description,
		num_cascades, num_cascades - 1);

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

	const GLsizei resolution = config -> resolution;
	init_texture_data(shadow_map_texture_type, (GLsizei[]) {resolution, resolution, num_cascades},
		GL_DEPTH_COMPONENT, internal_format, OPENGL_COLOR_CHANNEL_TYPE, NULL);

	////////// Creating the framebuffer

	GLuint framebuffer;
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(framebuffer_target, framebuffer);
	glFramebufferTexture(framebuffer_target, GL_DEPTH_ATTACHMENT, depth_layers, 0);
	glDrawBuffer(GL_NONE); glReadBuffer(GL_NONE); // Not drawing into or reading from any color buffers

	const GLchar* status_string;

	switch (glCheckFramebufferStatus(framebuffer_target)) {
		case GL_FRAMEBUFFER_COMPLETE:
			status_string = NULL;
			glBindFramebuffer(framebuffer_target, 0);
			break;

		#define COMPLETENESS_CASE(status) case GL_##status: status_string = #status; break

		COMPLETENESS_CASE(FRAMEBUFFER_UNDEFINED);
		COMPLETENESS_CASE(FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
		COMPLETENESS_CASE(FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
		COMPLETENESS_CASE(FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER);
		COMPLETENESS_CASE(FRAMEBUFFER_INCOMPLETE_READ_BUFFER);
		COMPLETENESS_CASE(FRAMEBUFFER_UNSUPPORTED);
		COMPLETENESS_CASE(FRAMEBUFFER_INCOMPLETE_MULTISAMPLE);
		COMPLETENESS_CASE(FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS);

		#undef COMPLETENESS_CASE

		default: status_string = "Unknown framebuffer error";
	}

	if (status_string != NULL)
		FAIL(CreateFramebuffer, "Could not create a framebuffer for this reason: '%s'", status_string);

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
	glDeleteFramebuffers(1, &shadow_context -> framebuffer);
	glDeleteSamplers(2, (GLuint[]) {shadow_context -> plain_depth_sampler, shadow_context -> depth_comparison_sampler});
}

void update_shadow_context(const CascadedShadowContext* const shadow_context,
	const Camera* const camera, const vec3 dir_to_light, const GLfloat aspect_ratio) {

	const GLfloat* const split_dists = shadow_context -> split_dists;
	mat4* const light_view_projection_matrices = shadow_context -> light_view_projection_matrices;

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

		get_light_view_projection(shadow_context, camera, dir_to_light, sub_near_clip,
			sub_far_clip, aspect_ratio, light_view_projection_matrices[i]);
	}
}

void enable_rendering_to_shadow_context(const CascadedShadowContext* const shadow_context) {
	const GLsizei resolution = shadow_context -> resolution;
	glViewport(0, 0, resolution, resolution);
	glBindFramebuffer(framebuffer_target, shadow_context -> framebuffer);
	glClear(GL_DEPTH_BUFFER_BIT);
}

void disable_rendering_to_shadow_context(const GLint screen_size[2]) {
	glBindFramebuffer(framebuffer_target, 0);
	glViewport(0, 0, screen_size[0], screen_size[1]);
}
