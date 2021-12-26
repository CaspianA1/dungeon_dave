#ifndef DRAWABLE_SET_C
#define DRAWABLE_SET_C

#include "headers/drawable_set.h"

void init_drawable_set_buffers(DrawableSet* const ds, const void* const vertex_data,
	const GLsizeiptr total_vertex_bytes, const GLenum object_buffer_usage) { // usage is GL_STATIC_DRAW or GL_DYNAMIC_DRAW

	GLuint vbo_and_ibo[2];
	glGenBuffers(2, vbo_and_ibo);

	ds -> dbo = vbo_and_ibo[0];
	glBindBuffer(GL_ARRAY_BUFFER, ds -> dbo);
	glBufferData(GL_ARRAY_BUFFER, total_vertex_bytes, vertex_data, GL_STATIC_DRAW);

	ds -> ibo = vbo_and_ibo[1];
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ds -> ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, total_index_bytes, NULL, GL_DYNAMIC_DRAW);

	ds -> ibo_ptr = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
}

void deinit_drawable_set(const DrawableSet* const ds) {

	/*
	deinit_list(ds -> objects);
	deinit_list(ds -> object_indices);
	glDeleteProgram(ds -> shader);
	deinit_texture(ds -> texture_set);

	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

	const GLuint buffers[2] = {ds -> dbo, ds -> ibo};
	glDeleteBuffers(2, buffers);
	*/
}

#endif
