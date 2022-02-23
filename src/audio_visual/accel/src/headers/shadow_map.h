#ifndef SHADOW_MAP_H
#define SHADOW_MAP_H

#include "utils.h"
#include "buffer_defs.h"
#include "batch_draw_context.h"

typedef struct {
	const struct {
		const GLuint depth_shader, light_model_view_projection_id;
	} shader_context;

	const struct {const GLuint texture, framebuffer;} depth_map;

	const GLsizei shadow_size[2];

	struct {
		vec3 pos, dir, up;
		mat4 model_view_projection;
	} light_context;

} ShadowMapContext;

// Excluded: enable_rendering_to_shadow_map, disable_rendering_to_shadow_map

ShadowMapContext init_shadow_map_context(
	const GLsizei shadow_width, const GLsizei shadow_height,
	const vec3 light_pos, const vec3 light_dir, vec3 light_up);

void deinit_shadow_map_context(const ShadowMapContext* const shadow_map_context);

void render_all_sectors_to_shadow_map(ShadowMapContext* const shadow_map_context,
	const BatchDrawContext* const sector_draw_context, const int screen_size[2]);

#endif
