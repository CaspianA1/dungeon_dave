#ifndef DRAWABLE_SET_H
#define DRAWABLE_SET_H

#include "utils.h"
#include "list.h"
#include "buffer_defs.h"

typedef struct {
	/* `objects: sectors, billboards, or something else.
	`object_indices`: a buffer of indices on the CPU, used for batching. If an object
	is in view, the indices of a given object are copied over to the gpu ibo. */
	List objects, object_indices; 
	/* TODO: init texture set from init_drawable; then, only one owner of texture_set.
	object_indices, ibo, and ibo_ptr may not be needed in all cases */

	/* `dbo` = data buffer object. Stores gpu-relevant data about each item in the drawable set.
	For sectors, it's a vbo with face vertices, and for billboards, it's a ubo with billboard attributes.
	`batch_modifier_ptr` may point to a memory-mapped version of the dbo, or the ibo (the dbo for billboards, as
	modified billboards are inserted directly there, and the ibo for sectors since selected sectors are chosen by indices. */
	GLuint dbo, ibo, shader, texture_set;
	buffer_index_t* batch_modifier_ptr;
} DrawableSet;

// TODO: generalized culling method for drawable set object

void init_drawable_set_buffers(DrawableSet* const ds, const void* const vertex_data,
	const GLsizeiptr total_vertex_bytes, const GLenum object_buffer_usage);

void deinit_drawable_set(const DrawableSet* const ds);

#endif