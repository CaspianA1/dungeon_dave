#ifndef CSM_H
#define CSM_H

#include "buffer_defs.h"
#include "batch_draw_context.h"

typedef struct {
	GLuint depth_texture, frame;
	GLsizei size[2];
} ShadowMapBuffers;

typedef struct {
	struct {
		GLfloat far_clip_dist;
		vec3 pos, dir;
		mat4 model_view_projection;
	} light;

	ShadowMapBuffers buffers;

	GLuint depth_shader;
} ShadowMapContext;

ShadowMapContext init_shadow_map_context(const GLsizei width,
	const GLsizei height, const vec3 light_pos, const GLfloat hori_angle,
	const GLfloat vert_angle, const GLfloat far_clip_dist);

void deinit_shadow_map_context(const ShadowMapContext* const s);

void update_shadow_map(ShadowMapContext* const shadow_map_context, const int screen_size[2],
	void (*const drawer) (const void* const), const void* const drawer_param);

#endif
