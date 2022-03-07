#ifndef SHADOW_MAP_H
#define SHADOW_MAP_H

#include "utils.h"
#include "buffer_defs.h"
#include "batch_draw_context.h"

// Shadow maps use variance shadow mapping in this implementation

typedef struct {
	const struct {
		const GLuint depth_shader;
		const GLint light_model_view_projection_id;
	} shader_context;

	const struct {
		const GLuint framebuffer, moment_texture, depth_render_buffer;
		const GLsizei size[2];
	} buffer_context;

	struct {
		vec3 pos, dir;
		mat4 model_view_projection;
	} light_context;
} ShadowMapContext;

// Excluded: get_model_view_projection_matrix_for_shadow_map, enable_rendering_to_shadow_map, disable_rendering_to_shadow_map

ShadowMapContext init_shadow_map_context(const GLsizei shadow_map_width,
	const GLsizei shadow_map_height, const vec3 light_pos,
	const GLfloat hori_angle, const GLfloat vert_angle);

void deinit_shadow_map_context(const ShadowMapContext* const shadow_map_context);

void render_all_sectors_to_shadow_map(
	ShadowMapContext* const shadow_map_context,
	const BatchDrawContext* const sector_draw_context,
	const int screen_size[2], const byte map_size[2]);

#endif
