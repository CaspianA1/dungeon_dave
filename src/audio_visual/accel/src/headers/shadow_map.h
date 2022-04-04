#ifndef SHADOW_MAP_H
#define SHADOW_MAP_H

#include "utils.h"
#include "buffer_defs.h"
#include "batch_draw_context.h"

#define MOMENT_TEXTURE_FORMAT GL_RGBA
#define MOMENT_TEXTURE_SIZED_FORMAT GL_RGBA32F

#define SHADOW_MAP_OUTPUT_TEXTURE_INDEX 0

/* Shadow maps use exponential variance shadow mapping + gaussian blur.
Note: a `pass` equals a stage in the shadow map generation process. */

typedef struct {
	struct {
		vec3 pos, dir;
		mat4 model_view_projection;
	} light_context;

	const struct {
		const GLsizei size[2];
		const GLuint framebuffer, ping_pong_textures[2];
	} buffer_context;

	const struct { // `depth_render_buffer` not in `buffer_context` b/c it's specific to the shadow pass
		const GLuint depth_render_buffer, depth_shader;
		const GLint light_model_view_projection_id;
	} shadow_pass;

	const struct {
		const GLuint blur_shader;
		const GLint blurring_horizontally_id;
	} blur_pass;
} ShadowMapContext;

/* Excluded: init_framebuffer, deinit_framebuffer,
get_model_view_projection_matrix_for_shadow_map, blur_shadow_map,
enable_rendering_to_shadow_map, disable_rendering_to_shadow_map */

#define deinit_framebuffer(f) glDeleteFramebuffers(1, &(f))

#define use_framebuffer(f) glBindFramebuffer(GL_FRAMEBUFFER, (f))
#define disable_current_framebuffer() glBindFramebuffer(GL_FRAMEBUFFER, 0)

ShadowMapContext init_shadow_map_context(const GLsizei shadow_map_width,
	const GLsizei shadow_map_height, const vec3 light_pos,
	const GLfloat hori_angle, const GLfloat vert_angle);

void deinit_shadow_map_context(ShadowMapContext* const shadow_map_context);

void render_all_sectors_to_shadow_map(
	ShadowMapContext* const shadow_map_context,
	const BatchDrawContext* const sector_draw_context,
	const int screen_size[2], const GLfloat far_clip_dist);

#endif
