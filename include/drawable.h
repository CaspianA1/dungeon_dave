#ifndef DRAWABLE_H
#define DRAWABLE_H

#include "buffer_defs.h"
#include "list.h"

// TOOD: once Drawable is used in more places, remove a lot of redundant #includes

struct Drawable; // The Drawable's shader will be bound when the uniform updater is called
typedef void (*const uniform_updater_t) (const struct Drawable* const drawable, const void* const param);

typedef struct {
	const GLenum triangle_mode;
	const GLuint vertex_spec, vertex_buffer, shader, diffuse_texture;
	const uniform_updater_t uniform_updater;
} Drawable;

typedef enum {
	OnlyDraw = 0,
	UseShaderPipeline = 1,
	BindVertexBuffer = 2,
	BindVertexSpec = 4
} DrawInvocationParam;

//////////

/*
Interface notes:
- Use `init_drawable_with_vertices` when you want to initialize a `drawable` with a set of vertices beforehand.
- If you do not want a vertex buffer or a vertex spec, use `init_drawable_without_vertices`.

- For the vertex list in `init_drawable_with_vertices`, the vertices in the list may be null. If so, a vertex buffer
	will be allocated for the vertices, but the vertex buffer will not have any elements inside it.
*/

////////// These are some utils that are commonly used with Drawable

void define_vertex_spec_index(const bool is_instanced, const bool treat_vertices_as_floats,
	const byte index, const byte num_components, const buffer_size_t stride,
	const buffer_size_t initial_offset, const GLenum typename);

// This is only for writing, not reading
void* init_gpu_memory_mapping(const GLenum target, const GLsizeiptr num_bytes, const bool discard_prev_contents);
#define deinit_gpu_memory_mapping glUnmapBuffer

////////// And these are the main Drawable functions

Drawable init_drawable_with_vertices(
	void (*const vertex_spec_definer) (void), const uniform_updater_t uniform_updater,
	const GLenum vertex_buffer_access, const GLenum triangle_mode,
	const List vertices, const GLuint shader, const GLuint diffuse_texture);

Drawable init_drawable_without_vertices(const uniform_updater_t uniform_updater,
	const GLenum triangle_mode, const GLuint shader, const GLuint diffuse_texture);

void deinit_drawable(const Drawable drawable);

void draw_drawable(const Drawable drawable,
	const buffer_size_t num_vertices, const buffer_size_t num_instances,
	const void* const uniform_updater_param, const byte invocation_params);

#endif
