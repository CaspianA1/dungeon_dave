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

	GLuint vertex_spec, texture_set, shader;
} BatchDrawContext;

typedef void (*const aabb_creator_t) (const byte* const cullable, vec3 aabb[2]);
typedef buffer_size_t (*const renderable_index_getter_t) (const byte* const cullable, const byte* const first_cullable);
typedef buffer_size_t (*const num_renderable_getter_t) (const byte* const cullable);

////////// Excluded: init_mapping_for_culled_batching

// This does not initialize or fill the CPU buffer with data; that's the caller's responsibility
void init_batch_draw_context_gpu_buffer(BatchDrawContext* const draw_context,
	const buffer_size_t num_drawable_things, const buffer_size_t drawable_thing_size);

void deinit_batch_draw_context(const BatchDrawContext* const draw_context);

// This returns how many of the objects were visible. It also binds the draw context's vbo.
buffer_size_t cull_from_frustum_into_gpu_buffer(
	const BatchDrawContext* const draw_context, const List cullable_objects,
	const vec4 frustum_planes[planes_per_frustum], const aabb_creator_t create_aabb,
	const renderable_index_getter_t get_renderable_index_from_cullable,
	const num_renderable_getter_t get_num_renderable_from_cullable);

#endif
