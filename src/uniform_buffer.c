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

/* Given an array of strings, along with a string count, this copies that buffer like this:
	1. It finds the total number of char bytes in the string array.
	2. It creates a contiguous buffer to store all of those bytes.
	3. It creates a pointer buffer that stores the beginning of each string in that buffer.
	4. It fills up the character buffer and the character pointer buffer.

To free this string array, you just have to 1. free the byte buffer by saying `free(*string_array)`,
and then 2. saying `free(string_array)` to free the pointer buffer.  */
static GLchar** copy_array_of_strings(const GLchar* const* const string_array, const buffer_size_t num_strings) {
	buffer_size_t total_char_count = 0;

	for (buffer_size_t i = 0; i < num_strings; i++)
		total_char_count += strlen(string_array[i]) + 1; // 1 extra for the null terminator

	GLchar
		*const char_buffer = malloc(total_char_count * sizeof(GLchar)),
		**const char_ptr_buffer = malloc(num_strings * sizeof(GLchar*));

	GLchar* curr_dest = char_buffer;
	for (buffer_size_t i = 0; i < num_strings; i++) {
		const GLchar* const curr_string = string_array[i];

		strcpy(curr_dest, curr_string);
		char_ptr_buffer[i] = curr_dest;
		curr_dest += strlen(curr_string) + 1;
	}

	return char_ptr_buffer;
}

UniformBuffer init_uniform_buffer(
	const bool updated_often, const GLchar* const block_name,
	const GLuint binding_point, const GLuint shader_using_uniform_block,
	const GLchar* const* const subvar_names, const buffer_size_t num_subvars) {

	GLuint buffer_id;
	glGenBuffers(1, &buffer_id);
	glBindBuffer(GL_UNIFORM_BUFFER, buffer_id);
	glBindBufferBase(GL_UNIFORM_BUFFER, binding_point, buffer_id);

	////////// First, getting the subvar indices

	GLuint* const subvar_indices = malloc(num_subvars * sizeof(GLuint));
	glGetUniformIndices(shader_using_uniform_block, (GLsizei) num_subvars, subvar_names, subvar_indices);

	for (buffer_size_t i = 0; i < num_subvars; i++) {
		if (subvar_indices[i] == GL_INVALID_INDEX) FAIL(InitializeShaderUniform,
			"Sub-variable '%s' in the uniform block '%s' is not present",
			subvar_names[i], block_name
		);
	}

	////////// Then, getting the byte offsets, the array stride, and the matrix stride, and freeing the subvar indices

	GLint* const subvar_gpu_byte_offsets = malloc(num_subvars * sizeof(GLint)), array_stride, matrix_stride;
	glGetActiveUniformsiv(shader_using_uniform_block, (GLsizei) num_subvars, subvar_indices, GL_UNIFORM_OFFSET, subvar_gpu_byte_offsets);
	glGetActiveUniformsiv(shader_using_uniform_block, (GLsizei) num_subvars, subvar_indices, GL_UNIFORM_ARRAY_STRIDE, &array_stride);
	glGetActiveUniformsiv(shader_using_uniform_block, (GLsizei) num_subvars, subvar_indices, GL_UNIFORM_MATRIX_STRIDE, &matrix_stride);

	free(subvar_indices);

	////////// After that, getting the block size in bytes, and allocating contents for the uniform buffer

	GLint block_size_in_bytes;

	glGetActiveUniformBlockiv(shader_using_uniform_block,
		safely_get_uniform_block_index(shader_using_uniform_block, block_name),
		GL_UNIFORM_BLOCK_DATA_SIZE, &block_size_in_bytes);

	glBufferData(GL_UNIFORM_BUFFER, block_size_in_bytes, NULL, updated_often ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

	//////////

	return (UniformBuffer) {
		.id = buffer_id, .binding_point = binding_point,
		.array_stride = (buffer_size_t) array_stride,
		.matrix_stride = (buffer_size_t) matrix_stride,
		.num_subvars = num_subvars, .block_name = block_name,
		.subvar_names = copy_array_of_strings(subvar_names, num_subvars),
		.subvar_gpu_byte_offsets = subvar_gpu_byte_offsets,
		.gpu_memory_mapping = NULL
	};
}

void deinit_uniform_buffer(const UniformBuffer* const buffer) {
	GLchar** subvar_names = buffer -> subvar_names;
	free(*subvar_names);
	free(subvar_names);

	free(buffer -> subvar_gpu_byte_offsets);
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
	glBindBuffer(GL_UNIFORM_BUFFER, buffer -> id);
	buffer -> gpu_memory_mapping = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
}

void disable_uniform_buffer_writing_batch(UniformBuffer* const buffer) {
	buffer -> gpu_memory_mapping = NULL;
	glUnmapBuffer(GL_UNIFORM_BUFFER);
}

static byte* get_base_pointer_for_subvar(const UniformBuffer* const buffer, const GLchar* const subvar_name) {
	byte* const gpu_memory_mapping = buffer -> gpu_memory_mapping;

	if (gpu_memory_mapping == NULL) FAIL(InitializeShaderUniform, "Cannot write subvar '%s'"
		" to uniform buffer because writing batch is not enabled", subvar_name
	);

	const buffer_size_t num_subvars = buffer -> num_subvars;
	GLchar* const* const subvar_names = buffer -> subvar_names;

	for (buffer_size_t i = 0; i < num_subvars; i++) {
		if (!strcmp(subvar_name, subvar_names[i]))
			return gpu_memory_mapping + buffer -> subvar_gpu_byte_offsets[i];
	}

	FAIL(InitializeShaderUniform,
		"Could not locate subvar '%s' within uniform block '%s'",
		subvar_name, buffer -> block_name
	);
}

void write_primitive_to_uniform_buffer(const UniformBuffer* const buffer,
	const GLchar* const subvar_name, const void* const value, const buffer_size_t size) {

	if (size > sizeof(vec4)) FAIL(InitializeShaderUniform, "Sizes greater than the size of vec4 (which is %zu)"
		" are not supported for `write_primitive_to_uniform_buffer`. The failing size was %u.", sizeof(vec4), size
	);

	memcpy(get_base_pointer_for_subvar(buffer, subvar_name), value, size);
}

void write_matrix_to_uniform_buffer(const UniformBuffer* const buffer, const GLchar* const subvar_name,
	const GLfloat* const matrix, const buffer_size_t size_per_column, const buffer_size_t num_columns) {

	const buffer_size_t stride = buffer -> matrix_stride;

	byte *dest = get_base_pointer_for_subvar(buffer, subvar_name);
	const byte* src = (byte*) matrix;

	for (buffer_size_t i = 0; i < num_columns; i++) {
		memcpy(dest, src, size_per_column);
		dest += size_per_column + stride;
		src += size_per_column;
	}
}

#endif
