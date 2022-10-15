#ifndef SHADOW_H
#define SHADOW_H

#include "utils/texture.h"
#include "utils/buffer_defs.h"
#include "camera.h"

/* This shadow mapping implementation employs cascaded shadow mapping with
exponential shadow mapping in order to get soft, filtered shadows for large scenes.
No mipmapping or summed area tables for filtering: just linear filtering medium-size
sample radius for the average occluder depth. */

static const TextureType shadow_map_texture_type = TexSet;

typedef struct {
	const GLuint framebuffer, depth_layers;
	const GLsizei resolution, num_cascades;
	const GLfloat sub_frustum_scale;

	GLfloat* const split_dists; // There are `num_cascades - 1` split dists
	mat4* const light_view_projection_matrices;
} CascadedShadowContext;

// Excluded: get_light_view_projection, init_csm_depth_layers, init_csm_framebuffer

void specify_cascade_count_before_any_shader_compilation(const GLsizei num_cascades);

CascadedShadowContext init_shadow_context(
	const GLfloat sub_frustum_scale, const GLfloat far_clip_dist,
	const GLfloat linear_split_weight, const GLsizei resolution,
	const GLsizei num_cascades, const GLsizei num_depth_buffer_bits);

void deinit_shadow_context(const CascadedShadowContext* const shadow_context);

void update_shadow_context(const CascadedShadowContext* const shadow_context,
	const Camera* const camera, const vec3 dir_to_light, const GLfloat aspect_ratio);

void enable_rendering_to_shadow_context(const CascadedShadowContext* const shadow_context);
void disable_rendering_to_shadow_context(const GLint screen_size[2]);

#endif
