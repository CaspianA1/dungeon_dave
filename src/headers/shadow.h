#ifndef SHADOW_H
#define SHADOW_H

#include "camera.h"
#include "list.h"

/* This shadow mapping implementation employs cascaded shadow mapping with
exponential shadow mapping in order to get soft shadows for large scenes. */

typedef struct {
	const GLuint depth_layers, framebuffer, depth_shader;
	const GLsizei resolution;

	const vec3 dir_to_light, sub_frustum_scale;

	const List split_dists, light_view_projection_matrices;
} CascadedShadowContext;

/* Excluded:
get_camera_sub_frustum_corners, get_light_view_and_projection,
get_sub_frustum_light_view_projection_matrix, init_csm_depth_layers, init_csm_framebuffer */

void specify_cascade_count_before_any_shader_compilation(const GLsizei num_cascades);

CascadedShadowContext init_shadow_context(
	const vec3 dir_to_light, const vec3 sub_frustum_scale,
	const GLfloat far_clip_dist, const GLfloat linear_split_weight,
	const GLsizei resolution, const GLsizei num_cascades);

void deinit_shadow_context(const CascadedShadowContext* const shadow_context);

void enable_rendering_to_shadow_context(const CascadedShadowContext* const shadow_context, const Camera* const camera);
void disable_rendering_to_shadow_context(const GLint screen_size[2]);

#endif
