#ifndef BATCH_DRAW_CONTEXT_H
#define BATCH_DRAW_CONTEXT_H

#include "list.h"

typedef struct {
	List cpu;
	GLuint gpu;
} BatchBufferPair;

typedef struct {
	BatchBufferPair object_buffers;
	GLuint texture_set, shader;
	buffer_index_t* gpu_buffer_ptr;
	// TODO: bb dynamic loading into gpu buffer
} BatchDrawContext; // Used for billboards

typedef struct { // In this, the gpu buffer ptr points to the index buffer
	BatchDrawContext c;
	BatchBufferPair index_buffers;
} IndexedBatchDrawContext;

void deinit_batch_draw_context(const BatchDrawContext* const draw_context, const byte gpu_buffer_ptr_is_for_indices);
void deinit_indexed_batch_draw_context(const IndexedBatchDrawContext* const draw_context);

#endif
