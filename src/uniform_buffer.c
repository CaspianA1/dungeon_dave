#ifndef UNIFORM_BUFFER_C
#define UNIFORM_BUFFER_C

#include "headers/uniform_buffer.h"
#include "headers/utils.h"

static GLuint safely_get_uniform_block_index(const GLuint shader, const GLchar* const block_name) {
	const GLuint block_index = glGetUniformBlockIndex(shader, block_name);

	if (block_index == GL_INVALID_INDEX) FAIL(InitializeShaderUniform,
		"Uniform block '%s' does not exist in shader", block_name
	);	

	return block_index;
}

UniformBuffer init_uniform_buffer(
	const bool updated_often, const GLchar* const block_name,
	const GLuint binding_point, const GLuint a_shader_using_the_uniform_block,
	const GLchar* const* const subvar_names, const GLsizei num_subvar_names) {

	GLuint buffer_id;
	glGenBuffers(1, &buffer_id);
	glBindBufferBase(GL_UNIFORM_BUFFER, binding_point, buffer_id);

	////////// First, getting the indices of the sub-vars

	GLuint* const subvar_indices = malloc((size_t) num_subvar_names * sizeof(GLuint));
	glGetUniformIndices(a_shader_using_the_uniform_block, num_subvar_names, subvar_names, subvar_indices);

	for (GLsizei i = 0; i < num_subvar_names; i++) {
		if (subvar_indices[i] == GL_INVALID_INDEX) FAIL(InitializeShaderUniform,
			"Sub-variable '%s' in the uniform block '%s' is not present",
			subvar_names[i], block_name
		);
	}

	////////// Then, getting the byte offsets, and freeing the subvar indices

	GLint* const subvar_byte_offsets = malloc((size_t) num_subvar_names * sizeof(GLint));
	glGetActiveUniformsiv(a_shader_using_the_uniform_block, num_subvar_names, subvar_indices, GL_UNIFORM_OFFSET, subvar_byte_offsets);

	free(subvar_indices);

	////////// After that, getting the block size in bytes, and allocating contents for the uniform buffer

	GLint block_size_in_bytes;

	glGetActiveUniformBlockiv(a_shader_using_the_uniform_block,
		safely_get_uniform_block_index(a_shader_using_the_uniform_block, block_name),
		GL_UNIFORM_BLOCK_DATA_SIZE, &block_size_in_bytes
	);

	glBindBuffer(GL_UNIFORM_BUFFER, buffer_id);	
	glBufferData(GL_UNIFORM_BUFFER, block_size_in_bytes, NULL, updated_often ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);	

	////////// And finally, allocating space on the heap for subvar names, and returning the block

	return (UniformBuffer) {
		.id = buffer_id, .binding_point = binding_point, .num_subvars = num_subvar_names,
		.block_name = block_name, .subvar_names = subvar_names, .subvar_byte_offsets = subvar_byte_offsets,
		.gpu_memory_mapping = NULL
	};
}

void deinit_uniform_buffer(const UniformBuffer* const buffer) {
	free(buffer -> subvar_byte_offsets);
	glDeleteBuffers(1, &buffer -> id);
}

void bind_uniform_buffer_to_shader(const UniformBuffer* const buffer, const GLuint shader) {
	const GLchar* const block_name = buffer -> block_name;

	const GLuint block_index = glGetUniformBlockIndex(shader, block_name);

	if (block_index == GL_INVALID_INDEX) FAIL(InitializeShaderUniform,
		"Uniform buffer name '%s' does not exist in shader", block_name
	);

	glUniformBlockBinding(shader, block_index, buffer -> binding_point);
}

void enable_uniform_buffer_writing_batch(UniformBuffer* const buffer) {
	buffer -> gpu_memory_mapping = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
}

void disable_uniform_buffer_writing_batch(UniformBuffer* const buffer) {
	buffer -> gpu_memory_mapping = NULL;
	glUnmapBuffer(GL_UNIFORM_BUFFER);
}

void write_to_uniform_buffer(const UniformBuffer* const buffer, const GLchar* const subvar_name, const void* const value, const size_t size) {
	byte* const gpu_memory_mapping = buffer -> gpu_memory_mapping;

	if (gpu_memory_mapping == NULL) FAIL(InitializeShaderUniform,
		"%s", "Cannot write to uniform buffer when writing batch is not enabled"
	);

	const GLint num_subvars = buffer -> num_subvars;
	const GLchar* const* const subvar_names = buffer -> subvar_names;
	for (GLint i = 0; i < num_subvars; i++) {
		if (!strcmp(subvar_name, subvar_names[i])) {
			const GLint byte_offset = buffer -> subvar_byte_offsets[i];
			memcpy(gpu_memory_mapping + byte_offset, value, size);
			return;
		}
	}

	FAIL(InitializeShaderUniform,
		"Could not locate sub-variable '%s' within uniform block '%s'",
		subvar_name, buffer -> block_name
	);
}

#endif
