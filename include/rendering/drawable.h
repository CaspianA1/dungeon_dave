#ifndef DRAWABLE_H
#define DRAWABLE_H

#include "utils/typedefs.h" // For OpenGL types + other typedefs
#include "utils/list.h" // For `List`

struct Drawable; // The Drawable's shader will be bound when the uniform updater is called
typedef void (*const uniform_updater_t) (const struct Drawable* const drawable, const void* const param);

typedef struct {
	const GLenum triangle_mode;
	const GLuint vertex_spec, vertex_buffer, shader, albedo_texture, normal_map;
	const uniform_updater_t uniform_updater;
} Drawable;

typedef enum {
	OnlyDraw = 0,
	UseShaderPipeline = 1,
	BindVertexSpec = 2
} DrawInvocationParam;

//////////

/*
Interface notes:
- Use `init_drawable_with_vertices` when you want to initialize a `drawable` with a set of vertices beforehand.
- If you do not want a vertex buffer or a vertex spec, use `init_drawable_without_vertices`.

- For the vertex list in `init_drawable_with_vertices`, the vertices in the list may be null. If so, a vertex buffer
	will be allocated for the vertices, but the vertex buffer will not have any elements inside it.

- If the uniform updater is null, it will not be invoked.
- And if the normal map set supplied to any constructor is 0, `glDeleteTextures` will not delete it.
*/

Drawable init_drawable_with_vertices(
	void (*const vertex_spec_definer) (void), const uniform_updater_t uniform_updater,
	const GLenum vertex_buffer_access, const GLenum triangle_mode,
	const List vertices, const GLuint shader, const GLuint albedo_texture,
	const GLuint normal_map);

Drawable init_drawable_without_vertices(const uniform_updater_t uniform_updater,
	const GLenum triangle_mode, const GLuint shader, const GLuint albedo_texture,
	const GLuint normal_map);

void deinit_drawable(const Drawable drawable);

void draw_drawable(const Drawable drawable,
	const buffer_size_t num_vertices, const buffer_size_t num_instances,
	const void* const uniform_updater_param, const byte invocation_params);

#endif
