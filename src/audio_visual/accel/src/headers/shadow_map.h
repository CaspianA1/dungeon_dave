#ifndef SHADOW_MAP_H
#define SHADOW_MAP_H

#include "utils.h"
#include "buffer_defs.h"
#include "batch_draw_context.h"

#define MOMENT_TEXTURE_FORMAT GL_RG
#define MOMENT_TEXTURE_SIZED_FORMAT GL_RG // GL_RG32F

#define SHADOW_MAP_BLUR_OUTPUT_TEXTURE_INDEX 1

// Shadow maps use variance shadow mapping + gaussian blur in this implementation

typedef struct {
	struct {
		vec3 pos, dir;
		mat4 model_view_projection;
	} light_context;

	const struct {
		const GLsizei buffer_size[2];
		const GLuint framebuffer, moment_texture, depth_render_buffer, depth_shader;
		const GLint light_model_view_projection_id;
	} shadow_pass; // A `pass` equals a stage in the shadow map generation process

	const struct { // TODO: reduce memory usage by perhaps using less textures and fbos
		const GLuint ping_pong_framebuffer, ping_pong_textures[2], blur_shader;
		const GLint blurring_horizontally_id;
	} blur_pass;
} ShadowMapContext;

/* Excluded:
init_framebuffer, deinit_framebuffer, deinit_framebuffers,
get_model_view_projection_matrix_for_shadow_map, blur_shadow_map,
enable_rendering_to_shadow_map, disable_rendering_to_shadow_map */

#define deinit_framebuffer(f) glDeleteFramebuffers(1, &(f))
#define deinit_framebuffers(length, fs) glDeleteFramebuffers((length), (fs)) // `fs` = framebuffer set

#define use_framebuffer(f) glBindFramebuffer(GL_FRAMEBUFFER, (f))
#define disable_current_framebuffer() glBindFramebuffer(GL_FRAMEBUFFER, 0)

ShadowMapContext init_shadow_map_context(const GLsizei shadow_map_width,
	const GLsizei shadow_map_height, const vec3 light_pos,
	const GLfloat hori_angle, const GLfloat vert_angle);

void deinit_shadow_map_context(ShadowMapContext* const shadow_map_context);

void render_all_sectors_to_shadow_map(
	ShadowMapContext* const shadow_map_context,
	const BatchDrawContext* const sector_draw_context,
	const int screen_size[2], const byte map_size[2]);

#endif
