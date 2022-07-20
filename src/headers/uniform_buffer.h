#ifndef UNIFORM_BUFFER_H
#define UNIFORM_BUFFER_H

#include "buffer_defs.h"

typedef struct {
	const GLuint id, binding_point;
	const GLint array_stride, matrix_stride;
	const buffer_size_t num_subvars;

	const GLchar* const block_name;
	GLchar** const subvar_names;
	GLint* const subvar_gpu_byte_offsets;

	byte* gpu_memory_mapping;
} UniformBuffer;

// Excluded: safely_get_uniform_block_index, copy_array_of_strings

UniformBuffer init_uniform_buffer(
	const bool updated_often, const GLchar* const block_name,
	const GLuint binding_point, const GLuint shader_using_uniform_block,
	const GLchar* const* const subvar_names, const buffer_size_t num_subvars);

void deinit_uniform_buffer(const UniformBuffer* const buffer);

void bind_uniform_buffer_to_shader(const UniformBuffer* const buffer, const GLuint shader);

void enable_uniform_buffer_writing_batch(UniformBuffer* const buffer);
void disable_uniform_buffer_writing_batch(UniformBuffer* const buffer);
void write_to_uniform_buffer(const UniformBuffer* const buffer, const GLchar* const subvar_name, const void* const value, const size_t size);

#endif
