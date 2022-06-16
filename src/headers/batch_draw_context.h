#ifndef BATCH_DRAW_CONTEXT_H
#define BATCH_DRAW_CONTEXT_H

#include "../../include/glad/glad.h"
#include "list.h"

// TODO: deprecate this once Drawable is done

typedef struct {
	struct {
		List cpu;
		GLuint gpu;
	} buffers;

	GLuint vertex_spec, texture_set, shader;
} BatchDrawContext;

void init_batch_draw_context_gpu_buffer(BatchDrawContext* const draw_context,
	const buffer_size_t num_drawable_things, const buffer_size_t drawable_thing_size);

void deinit_batch_draw_context(const BatchDrawContext* const draw_context);

// This assumes that the current bound buffer is the draw context's buffer
void* init_mapping_for_culled_batching(const BatchDrawContext* const draw_context);
#define deinit_current_mapping_for_culled_batching() glUnmapBuffer(GL_ARRAY_BUFFER)

#endif
