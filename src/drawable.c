#ifndef DRAWABLE_C
#define DRAWABLE_C

#include "headers/drawable.h"

Drawable init_drawable(
	void (*const vertex_spec_definer) (void), const uniform_updater_t uniform_updater,
	const GLenum vertex_buffer_access, const GLenum triangle_mode,
	const List vertices, const GLuint shader, const GLuint diffuse_texture) {

	GLuint vertex_buffer, vertex_spec;

	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.length * vertices.item_size, vertices.data, vertex_buffer_access);

	glGenVertexArrays(1, &vertex_spec);
	glBindVertexArray(vertex_spec);
	vertex_spec_definer();

	return (Drawable) {
		triangle_mode, vertex_spec, vertex_buffer,
		shader, diffuse_texture, uniform_updater
	};
}

Drawable init_drawable_without_vertices(const uniform_updater_t uniform_updater,
	const GLenum triangle_mode, const GLuint shader, const GLuint diffuse_texture) {

	return (Drawable) {triangle_mode, 0, 0, shader, diffuse_texture, uniform_updater};
}

void deinit_drawable(const Drawable drawable) {
	glDeleteTextures(1, &drawable.diffuse_texture);
	glDeleteProgram(drawable.shader);

	glDeleteBuffers(1, &drawable.vertex_buffer);
	glDeleteVertexArrays(1, &drawable.vertex_spec);
}

void draw_drawable(const Drawable drawable, const GLsizei num_vertices_to_draw, const void* const uniform_updater_param) {
	glUseProgram(drawable.shader);
	drawable.uniform_updater((struct Drawable*) &drawable, uniform_updater_param);

	if (drawable.vertex_buffer != 0 && drawable.vertex_spec != 0) {
		glBindBuffer(GL_ARRAY_BUFFER, drawable.vertex_buffer);
		glBindVertexArray(drawable.vertex_spec);
	}

	glDrawArrays(drawable.triangle_mode, 0, num_vertices_to_draw);
}

#endif
