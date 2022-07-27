#include "drawable.h"

Drawable init_drawable_with_vertices(
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

	if (drawable.vertex_buffer != 0) glDeleteBuffers(1, &drawable.vertex_buffer);
	if (drawable.vertex_spec != 0) glDeleteVertexArrays(1, &drawable.vertex_spec);
}

void draw_drawable(const Drawable drawable,
	const buffer_size_t num_vertices, const buffer_size_t num_instances,
	const void* const uniform_updater_param, const byte invocation_params) {

	if (invocation_params & UseShaderPipeline) {
		glUseProgram(drawable.shader);
		drawable.uniform_updater((struct Drawable*) &drawable, uniform_updater_param);
	}

	if (invocation_params & BindVertexBuffer) glBindBuffer(GL_ARRAY_BUFFER, drawable.vertex_buffer);
	if (invocation_params & BindVertexSpec) glBindVertexArray(drawable.vertex_spec);

	if (num_instances == 0) glDrawArrays(drawable.triangle_mode, 0, (GLsizei) num_vertices);
	else glDrawArraysInstanced(drawable.triangle_mode, 0, (GLsizei) num_vertices, (GLsizei) num_instances);
}
