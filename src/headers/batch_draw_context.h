#ifndef BATCH_DRAW_CONTEXT_H
#define BATCH_DRAW_CONTEXT_H

#include "../../include/glad/glad.h"
#include "list.h"
#include "buffer_defs.h"

// TODO: deprecate this once Drawable is done

typedef struct {
	struct {
		List cpu;
		GLuint gpu;
	} buffers;

	const GLuint vertex_spec, texture_set, shader;
} BatchDrawContext;

void deinit_batch_draw_context(const BatchDrawContext* const draw_context);

#endif
