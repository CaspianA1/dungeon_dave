#ifndef CSM_H
#define CSM_H

#include "camera.h"
#include "list.h"

typedef struct {
	GLuint depth_layers, framebuffer, depth_shader;
	GLsizei resolution[2];

	vec3 light_dir, sub_frustum_scale;
	List light_view_projection_matrices, split_dists;
} CascadedShadowContext;

// Excluded: apply_texel_snapping, get_sub_frustum_light_view_projection_matrix, init_csm_depth_layers, init_csm_framebuffer

CascadedShadowContext init_shadow_context(const vec3 light_dir, const vec3 sub_frustum_scale,
	const GLfloat far_clip_dist, const GLfloat linear_split_weight, const GLsizei width,
	const GLsizei height, const GLsizei num_layers);

void deinit_shadow_context(const CascadedShadowContext* const shadow_context);

void draw_to_shadow_context(const CascadedShadowContext* const shadow_context, const Camera* const camera,
	const GLint screen_size[2], void (*const drawer) (const void* const), const void* const drawer_param);

#endif
