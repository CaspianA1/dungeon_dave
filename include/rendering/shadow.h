#ifndef SHADOW_H
#define SHADOW_H

#include "utils/typedefs.h" // For OpenGL types + other typedefs
#include "utils/texture.h" // For `TextureType`
#include "utils/cglm_include.h" // For `mat4`
#include "camera.h" // For `Camera`

/* This shadow mapping implementation employs cascaded shadow mapping with
exponential shadow mapping in order to get soft, filtered shadows for large scenes.
No mipmapping or summed area tables for filtering: just linear filtering, and a medium-size
sample radius to find the average occluder depth.

Volumetric lighting is also calculated with the shadow map cascades. The technique is from
https://developer.nvidia.com/gpugems/gpugems3/part-ii-light-and-shadows/chapter-13-volumetric-light-scattering-post-process,
but this method uses the scene's shadow map, instead of a screen-space depth map.

Because volumetric lighting requires a series of depth comparisons for good results,
instead of setting sampling parameters for the depth layers, the sampling parameters
are split into two different sampler objects; one that fetches raw depth values, and one
that fetches four averaged bilinear depth comparisons.  */

static const TextureType shadow_map_texture_type = TexSet;

typedef struct {
	const byte num_cascades, num_depth_buffer_bits;
	const GLsizei resolution;
	const GLfloat sub_frustum_scale, linear_split_weight;
} CascadedShadowContextConfig;

typedef struct {
	const GLuint
		framebuffer, depth_layers,
		plain_depth_sampler, depth_comparison_sampler;

	const GLsizei resolution;
	const byte num_cascades;
	const GLfloat sub_frustum_scale;

	GLfloat* const split_dists; // There are `num_cascades - 1` split dists
	mat4* const light_view_projection_matrices; // And `num_cascades` projection matrices
} CascadedShadowContext;

// Excluded: get_light_view_projection

void specify_cascade_count_before_any_shader_compilation(
	const byte opengl_major_minor_version[2], const byte num_cascades);

CascadedShadowContext init_shadow_context(const CascadedShadowContextConfig* const config, const GLfloat far_clip_dist);
void deinit_shadow_context(const CascadedShadowContext* const shadow_context);

void update_shadow_context(const CascadedShadowContext* const shadow_context,
	const Camera* const camera, const vec3 dir_to_light, const GLfloat aspect_ratio);

void enable_rendering_to_shadow_context(const CascadedShadowContext* const shadow_context);
void disable_rendering_to_shadow_context(const GLint screen_size[2]);

#endif
