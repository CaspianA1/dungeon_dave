#ifndef BATCH_DRAW_CONTEXT_H
#define BATCH_DRAW_CONTEXT_H

#include "utils.h"
#include "list.h"

typedef struct {
	struct {
		List cpu;
		GLuint gpu;
	} buffers;

	GLuint texture_set, shader;
} BatchDrawContext;

void init_batch_draw_context_gpu_buffer(BatchDrawContext* const draw_context,
	const buffer_size_t num_drawable_things, const buffer_size_t drawable_thing_size);

void deinit_batch_draw_context(const BatchDrawContext* const draw_context);

#endif
