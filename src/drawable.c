#include "drawable.h"

////////// Utils commonly used with Drawable

void define_vertex_spec_index(const bool is_instanced, const bool treat_vertices_as_floats,
	const byte index, const byte num_components, const buffer_size_t stride,
	const buffer_size_t initial_offset, const GLenum typename) {

	glEnableVertexAttribArray(index);
	if (is_instanced) glVertexAttribDivisor(index, 1);

	const GLsizei cast_stride = (GLsizei) stride;
	const void* const cast_initial_offset = (void*) (size_t) initial_offset;

	if (treat_vertices_as_floats) glVertexAttribPointer(index, num_components, typename, GL_FALSE, cast_stride, cast_initial_offset);
	else glVertexAttribIPointer(index, num_components, typename, cast_stride, cast_initial_offset);
}

// TODO: possibly add `GL_MAP_UNSYNCHRONIZED_BIT`, if possible? Test on Chromebook.
void* init_gpu_memory_mapping(const GLenum target, const GLsizeiptr num_bytes, const bool discard_prev_contents) {
	const GLbitfield flags = GL_MAP_WRITE_BIT | (GL_MAP_INVALIDATE_BUFFER_BIT * discard_prev_contents);
	void* const mapping = glMapBufferRange(target, 0, num_bytes, flags);
	if (mapping == NULL) FAIL(InitializeGPUMemoryMapping, "%s", "Initializing a GPU memory mapping was not possible");
	return mapping;
}

////////// The main Drawable functions

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
