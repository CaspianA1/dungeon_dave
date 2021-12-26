#ifndef BATCH_DRAW_CONTEXT_C
#define BATCH_DRAW_CONTEXT_C

#include "headers/batch_draw_context.h"
#include "list.c"

void deinit_batch_draw_context(const BatchDrawContext* const draw_context, const byte gpu_buffer_ptr_is_for_indices) {
	deinit_list(draw_context -> object_buffers.cpu);

	glUnmapBuffer(gpu_buffer_ptr_is_for_indices ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER);
	glDeleteBuffers(1, &draw_context -> object_buffers.gpu);

	deinit_texture(draw_context -> texture_set);
	glDeleteProgram(draw_context -> shader);
}

void deinit_indexed_batch_draw_context(const IndexedBatchDrawContext* const draw_context) {
	deinit_list(draw_context -> index_buffers.cpu);
	deinit_batch_draw_context(&draw_context -> c, 1);
	glDeleteBuffers(1, &draw_context -> index_buffers.gpu);
}

#endif
