#ifndef DRAWABLE_H
#define DRAWABLE_H

#include "buffer_defs.h"
#include "list.h"

// TOOD: once Drawable is used in more places, remove a lot of redundant #includes

struct Drawable; // The Drawable's shader will be bound when this is called
typedef void (*const uniform_updater_t) (const struct Drawable* const drawable, const void* const param);

typedef struct {
	const bool using_triangle_strip;
	const GLuint vertex_spec, vertex_buffer, shader, diffuse_texture;
	const uniform_updater_t uniform_updater;
} Drawable;

//////////

Drawable init_drawable(
	void (*const vertex_spec_definer) (void), const uniform_updater_t uniform_updater,
	const bool will_modify_vertices, const bool using_triangle_strip,
	const List vertices, const GLuint shader, const GLuint diffuse_texture);

void deinit_drawable(const Drawable drawable);
void draw_drawable(const Drawable drawable, const GLsizei num_vertices_to_draw, const void* const uniform_updater_param);

#endif
