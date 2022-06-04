#ifndef CSM_H
#define CSM_H

#include "camera.h"
#include "list.h"

typedef struct {
	const GLuint depth_layers, framebuffer, depth_shader;
	const GLsizei resolution[2];

	const GLfloat z_scale;
	const vec3 light_dir;
	const List light_view_projection_matrices;
} CascadedShadowContext;

// Excluded: get_csm_light_view_projection_matrix, init_csm_depth_layers, init_csm_framebuffer

CascadedShadowContext init_csm_context(const vec3 light_dir, const GLfloat z_scale,
	const GLsizei width, const GLsizei height, const GLsizei num_layers);

void deinit_csm_context(const CascadedShadowContext* const csm_context);

void draw_to_csm_context(const CascadedShadowContext* const csm_context, const Camera* const camera,
	const GLint screen_size[2], void (*const drawer) (const void* const), const void* const drawer_param);

#endif
