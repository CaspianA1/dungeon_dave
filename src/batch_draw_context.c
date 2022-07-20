#ifndef BATCH_DRAW_CONTEXT_C
#define BATCH_DRAW_CONTEXT_C

#include "headers/batch_draw_context.h"
#include "headers/shader.h"
#include "headers/texture.h"

void deinit_batch_draw_context(const BatchDrawContext* const draw_context) {
	deinit_list(draw_context -> buffers.cpu);
	deinit_gpu_buffer(draw_context -> buffers.gpu);
	deinit_vertex_spec(draw_context -> vertex_spec);
	deinit_texture(draw_context -> texture_set);
	deinit_shader(draw_context -> shader);
}

//////////

#endif
