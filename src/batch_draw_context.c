#ifndef BATCH_DRAW_CONTEXT_C
#define BATCH_DRAW_CONTEXT_C

#include "headers/batch_draw_context.h"
#include "headers/shader.h"
#include "headers/texture.h"

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

//////////

static void* init_mapping_for_culled_batching(const BatchDrawContext* const draw_context) {
	const List* const cpu_buffer = &draw_context -> buffers.cpu;
	const GLsizeiptr num_bytes = cpu_buffer -> length * cpu_buffer -> item_size;

	/* Flags, explained:
	1. Write only (only writing from cpu to gpu buffer, no other operations).
	2. Whole previous contents of buffer can be discarded, since the batch is written from scratch each time.
	3. No other GPU commands depend on the buffer contents, so no implicit synchronization! */
	const GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT;

	return glMapBufferRange(GL_ARRAY_BUFFER, 0, num_bytes, flags);
}

//////////

buffer_size_t cull_from_frustum_into_gpu_buffer(
	const BatchDrawContext* const draw_context, const List cullable_objects,
	const vec4 frustum_planes[planes_per_frustum], const aabb_creator_t create_aabb,
	const renderable_index_getter_t get_renderable_index_from_cullable,
	const num_renderable_getter_t get_num_renderable_from_cullable) {

	use_vertex_buffer(draw_context -> buffers.gpu);

	const byte* const out_of_bounds_cullable = ptr_to_list_index(&cullable_objects, cullable_objects.length);

	byte* const renderable_gpu_buffer = init_mapping_for_culled_batching(draw_context);
	const List* const renderable_cpu_buffer = &draw_context -> buffers.cpu;
	const buffer_size_t renderable_size = renderable_cpu_buffer -> item_size;

	//////////

	buffer_size_t total_num_visible = 0;

	for (const byte* cullable = cullable_objects.data; cullable < out_of_bounds_cullable; cullable += cullable_objects.item_size) {
		const buffer_size_t initial_renderable_index_in_span = get_renderable_index_from_cullable(cullable, cullable_objects.data);

		buffer_size_t num_visible_in_group = 0;
		vec3 curr_aabb[2];

		while (cullable < out_of_bounds_cullable) {
			create_aabb(cullable, curr_aabb);
			if (!glm_aabb_frustum(curr_aabb, (vec4*) frustum_planes)) break;

			num_visible_in_group += get_num_renderable_from_cullable(cullable);
			cullable += cullable_objects.item_size;
		}

		if (num_visible_in_group != 0) {
			byte* const dest = renderable_gpu_buffer + total_num_visible * renderable_size;
			const byte* const src = ptr_to_list_index(renderable_cpu_buffer, initial_renderable_index_in_span);
			const buffer_size_t num_bytes = num_visible_in_group * renderable_size;

			memcpy(dest, src, num_bytes);

			total_num_visible += num_visible_in_group;
		}
	}

	//////////

	glUnmapBuffer(GL_ARRAY_BUFFER);
	return total_num_visible;
}

#endif
