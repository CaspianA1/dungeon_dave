#ifndef UNIFORM_BUFFER_H
#define UNIFORM_BUFFER_H

#include "utils/typedefs.h" // For OpenGL types + other typedefs
#include "list.h" // For `List`
#include <stdbool.h> // For `bool`

typedef struct {
	const GLuint id, binding_point;
	byte* gpu_memory_mapping;

	const struct {
		const buffer_size_t size;
		const GLchar* const name;
	} block;

	const struct {
		const buffer_size_t count;
		GLint *const sizes, *const byte_offsets, *const array_strides, *const matrix_strides;
		const GLchar* const* const names;
	} subvars;
} UniformBuffer;

/* Excluded: safely_get_uniform_block_index, check_if_is_array,
check_primitive_size, check_matrix_size, check_array_length, get_subvar_metadata */

/* This expects that lifetime of `subvar_names` is longer than the lifetime of the
uniform buffer (so the subvar names should be on the heap, or in static memory). */
UniformBuffer init_uniform_buffer(const GLenum usage,
	const GLchar* const block_name, const GLuint shader_using_uniform_block,
	const GLchar* const* const subvar_names, const buffer_size_t num_subvars);

void deinit_uniform_buffer(const UniformBuffer* const buffer);
void bind_uniform_buffer_to_shader(const UniformBuffer* const buffer, const GLuint shader);
void enable_uniform_buffer_writing_batch(UniformBuffer* const buffer, const bool discard_prev_contents);
void disable_uniform_buffer_writing_batch(UniformBuffer* const buffer);

void write_primitive_to_uniform_buffer(const UniformBuffer* const buffer,
	const GLchar* const subvar_name, const void* const primitive, const buffer_size_t size);

void write_matrix_to_uniform_buffer(const UniformBuffer* const buffer, const GLchar* const subvar_name,
	const GLfloat* const matrix, const buffer_size_t column_size, const buffer_size_t num_columns);

void write_array_of_primitives_to_uniform_buffer(const UniformBuffer* const buffer,
	const GLchar* const subvar_name, const List primitives);

void write_array_of_matrices_to_uniform_buffer(const UniformBuffer* const buffer,
	const GLchar* const subvar_name, const GLfloat* const* const matrices,
	const buffer_size_t num_matrices, const buffer_size_t column_size,
	const buffer_size_t num_columns);

#endif
