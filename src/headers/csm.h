#ifndef CSM_H
#define CSM_H

#include "camera.h"
#include "list.h"

typedef struct {
	GLsizei resolution[2], num_layers;
	GLfloat linear_split_weight, z_scale;
	struct {GLfloat hori, vert;} light_angles; // These are in degrees
} CascadedShadowSpec;

typedef struct {
	GLuint depth_layers, framebuffer, depth_shader;

	vec3 light_dir;
	List light_view_projection_matrices, split_dists;

	CascadedShadowSpec spec;
} CascadedShadowContext;

// Excluded: get_csm_light_view_projection_matrix, init_depth_layers, init_framebuffer_with_depth_layers

CascadedShadowContext init_shadow_context(const CascadedShadowSpec spec, const GLfloat far_clip_dist);

void deinit_shadow_context(const CascadedShadowContext* const shadow_context);

void draw_to_shadow_context(const CascadedShadowContext* const shadow_context, const Camera* const camera,
	const GLint screen_size[2], void (*const drawer) (const void* const), const void* const drawer_param);

#endif
