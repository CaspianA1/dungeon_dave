#include "utils/uniform_buffer.h"
#include "utils/alloc.h"
#include "utils/utils.h"
#include "rendering/drawable.h"

static const GLchar* const max_primitive_size_name = "dvec4";
static const size_t max_primitive_size = sizeof(GLdouble[4]); // The size of a dvec4

////////// This part concerns initialization, deinitialization, binding a uniform buffer to a shader, and the mapping of a buffer

static GLuint safely_get_uniform_block_index(const GLuint shader, const GLchar* const block_name) {
	const GLuint block_index = glGetUniformBlockIndex(shader, block_name);

	if (block_index == GL_INVALID_INDEX) FAIL(InitializeShaderUniform,
		"Uniform block '%s' does not exist in shader", block_name
	);	

	return block_index;
}

UniformBuffer init_uniform_buffer(
	const GLenum usage, const GLchar* const block_name,
	const GLuint binding_point, const GLuint shader_using_uniform_block,
	const GLchar* const* const subvar_names, const buffer_size_t num_subvars) {

	GLuint buffer_id;
	glGenBuffers(1, &buffer_id);
	glBindBuffer(GL_UNIFORM_BUFFER, buffer_id);
	glBindBufferBase(GL_UNIFORM_BUFFER, binding_point, buffer_id);

	////////// First, getting the subvar indices

	GLuint* const subvar_indices = alloc(num_subvars, sizeof(GLuint));
	glGetUniformIndices(shader_using_uniform_block, (GLsizei) num_subvars, subvar_names, subvar_indices);

	for (buffer_size_t i = 0; i < num_subvars; i++) {
		if (subvar_indices[i] == GL_INVALID_INDEX) FAIL(InitializeShaderUniform,
			"Subvar '%s' in the uniform block '%s' is not present",
			subvar_names[i], block_name
		);
	}

	////////// Then, getting the byte offsets, the array stride, and the matrix stride, and freeing the subvar indices

	GLint* const subvar_metadata_buffer = alloc(num_subvars * 3, sizeof(GLint));

	GLint
		*const subvar_byte_offsets = subvar_metadata_buffer,
		*const subvar_array_strides = subvar_metadata_buffer + num_subvars,
		*const subvar_matrix_strides = subvar_metadata_buffer + (num_subvars << 1);

	glGetActiveUniformsiv(shader_using_uniform_block, (GLsizei) num_subvars, subvar_indices, GL_UNIFORM_OFFSET, subvar_byte_offsets);
	glGetActiveUniformsiv(shader_using_uniform_block, (GLsizei) num_subvars, subvar_indices, GL_UNIFORM_ARRAY_STRIDE, subvar_array_strides);
	glGetActiveUniformsiv(shader_using_uniform_block, (GLsizei) num_subvars, subvar_indices, GL_UNIFORM_MATRIX_STRIDE, subvar_matrix_strides);

	dealloc(subvar_indices);

	////////// After that, getting the block size in bytes, and allocating contents for the uniform buffer

	GLint block_size_in_bytes;

	glGetActiveUniformBlockiv(shader_using_uniform_block,
		safely_get_uniform_block_index(shader_using_uniform_block, block_name),
		GL_UNIFORM_BLOCK_DATA_SIZE, &block_size_in_bytes);

	glBufferData(GL_UNIFORM_BUFFER, block_size_in_bytes, NULL, usage);

	////////// And finally, returning the uniform buffer

	return (UniformBuffer) {
		.id = buffer_id, .binding_point = binding_point,
		.gpu_memory_mapping = NULL,

		.block = {
			.size = (buffer_size_t) block_size_in_bytes,
			.name = block_name
		},

		.subvars = {
			.count = num_subvars,
			.byte_offsets = subvar_byte_offsets,
			.array_strides = subvar_array_strides,
			.matrix_strides = subvar_matrix_strides,
			.names = subvar_names
		}
	};
}

void deinit_uniform_buffer(const UniformBuffer* const buffer) {
	/* The subvar metadata, which includes the byte offsets, and array and matrix strides, are all allocated
	in one block together, so freeing the byte offsets (which is the beginning of the block) frees all of them. */
	dealloc(buffer -> subvars.byte_offsets);
	glDeleteBuffers(1, &buffer -> id);
}

void bind_uniform_buffer_to_shader(const UniformBuffer* const buffer, const GLuint shader) {
	glUniformBlockBinding(shader, safely_get_uniform_block_index(shader, buffer -> block.name), buffer -> binding_point);
}

void enable_uniform_buffer_writing_batch(UniformBuffer* const buffer, const bool discard_prev_contents) {
	glBindBuffer(GL_UNIFORM_BUFFER, buffer -> id);
	buffer -> gpu_memory_mapping = init_gpu_memory_mapping(GL_UNIFORM_BUFFER, buffer -> block.size, discard_prev_contents);
}

void disable_uniform_buffer_writing_batch(UniformBuffer* const buffer) {
	buffer -> gpu_memory_mapping = NULL;
	deinit_gpu_memory_mapping(GL_UNIFORM_BUFFER);
}

////////// This part concerns writing data to the uniform buffer

static void check_primitive_size(const buffer_size_t size, const GLchar* const function_name) {
	if (size == 0) FAIL(InitializeShaderUniform, "A primitive size of 0 when calling `%s` is not supported", function_name);

	else if (size > max_primitive_size) FAIL(InitializeShaderUniform,
		"Sizes greater than the size of %s (which is %zu) are not supported"
		" for `%s`. The failing size was %u", max_primitive_size_name,
		max_primitive_size, function_name, size
	);
}

static void check_matrix_size(const buffer_size_t column_size,
	const buffer_size_t num_columns, const GLchar* const function_name) {

	check_primitive_size(column_size, function_name);

	if (num_columns == 0 || num_columns > 4) FAIL(InitializeShaderUniform,
		"A column count of %u when calling `%s` is not supported",
		num_columns, function_name
	);
}

// If either stride is null, it will not be written to.
static void get_subvar_metadata(const UniformBuffer* const buffer, const GLchar* const subvar_name,
	byte** const gpu_memory_dest, GLint* const array_stride, GLint* const matrix_stride) {

	byte* const gpu_memory_mapping = buffer -> gpu_memory_mapping;

	if (gpu_memory_mapping == NULL) FAIL(InitializeShaderUniform, "Cannot write subvar '%s'"
		" to uniform buffer because writing batch is not enabled", subvar_name
	);

	const buffer_size_t num_subvars = buffer -> subvars.count;
	const GLchar* const* const subvar_names = buffer -> subvars.names;

	for (buffer_size_t i = 0; i < num_subvars; i++) {
		if (!strcmp(subvar_name, subvar_names[i])) {
			// In some cases, need the gpu buffer ptr. Never need the actual index. Also need the _ in some cases.
			*gpu_memory_dest = buffer -> gpu_memory_mapping + buffer -> subvars.byte_offsets[i];

			if (array_stride != NULL) *array_stride = buffer -> subvars.array_strides[i];
			if (matrix_stride != NULL) *matrix_stride = buffer -> subvars.matrix_strides[i];

			return;
		}
	}

	FAIL(InitializeShaderUniform,
		"Could not locate subvar '%s' within uniform block '%s'",
		subvar_name, buffer -> block.name
	);
}

void write_primitive_to_uniform_buffer(const UniformBuffer* const buffer,
	const GLchar* const subvar_name, const void* const primitive, const buffer_size_t size) {

	check_primitive_size(size, "write_primitive_to_uniform_buffer");

	byte* dest;
	get_subvar_metadata(buffer, subvar_name, &dest, NULL, NULL);
	memcpy(dest, primitive, size);
}

void write_matrix_to_uniform_buffer(const UniformBuffer* const buffer, const GLchar* const subvar_name,
	const GLfloat* const matrix, const buffer_size_t column_size, const buffer_size_t num_columns) {

	check_matrix_size(column_size, num_columns, "write_matrix_to_uniform_buffer");

	byte* dest;
	GLint matrix_stride;
	get_subvar_metadata(buffer, subvar_name, &dest, NULL, &matrix_stride);

	const byte* src = (byte*) matrix;

	for (buffer_size_t i = 0; i < num_columns; i++) {
		memcpy(dest, src, column_size);
		dest += matrix_stride;
		src += column_size;
	}
}

void write_array_of_primitives_to_uniform_buffer(const UniformBuffer* const buffer,
	const GLchar* const subvar_name, const List primitives) {

	check_primitive_size(primitives.item_size, "write_array_of_primitives_to_uniform_buffer");

	byte* dest;
	GLint array_stride;
	get_subvar_metadata(buffer, subvar_name, &dest, &array_stride, NULL);

	LIST_FOR_EACH(0, &primitives, primitive, _,
		memcpy(dest, primitive, primitives.item_size);
		dest += array_stride;
	);
}

void write_array_of_matrices_to_uniform_buffer(const UniformBuffer* const buffer,
	const GLchar* const subvar_name, const GLfloat* const* const matrices,
	const buffer_size_t num_matrices, const buffer_size_t column_size,
	const buffer_size_t num_columns) {

	check_matrix_size(column_size, num_columns, "write_matrix_to_uniform_buffer");

	byte* dest;
	GLint matrix_stride;
	get_subvar_metadata(buffer, subvar_name, &dest, NULL, &matrix_stride);

	const byte* src = (byte*) matrices;

	for (buffer_size_t i = 0; i < num_matrices; i++) {
		for (buffer_size_t i = 0; i < num_columns; i++) {
			memcpy(dest, src, column_size);
			dest += matrix_stride;
			src += column_size;
		}
	}
}