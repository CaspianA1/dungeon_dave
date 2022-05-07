#ifndef BATCH_DRAW_CONTEXT_C
#define BATCH_DRAW_CONTEXT_C

#include "headers/batch_draw_context.h"
#include "headers/texture.h"

// This does not initialize or fill the CPU buffer with data; that's the caller's responsibility
void init_batch_draw_context_gpu_buffer(BatchDrawContext* const draw_context,
	const buffer_size_t num_drawable_things, const buffer_size_t drawable_thing_size) {

	const GLuint gpu_buffer = init_gpu_buffer();
	use_vertex_buffer(gpu_buffer);
	glBufferData(GL_ARRAY_BUFFER, num_drawable_things * drawable_thing_size, NULL, GL_DYNAMIC_DRAW);
	draw_context -> buffers.gpu = gpu_buffer;
}

void deinit_batch_draw_context(const BatchDrawContext* const draw_context) {
	deinit_list(draw_context -> buffers.cpu);
	deinit_gpu_buffer(draw_context -> buffers.gpu);
	deinit_vertex_spec(draw_context -> vertex_spec);
	deinit_texture(draw_context -> texture_set);
	deinit_shader(draw_context -> shader);
}

#endif
