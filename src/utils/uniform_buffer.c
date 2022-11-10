#include "utils/uniform_buffer.h"
#include "utils/alloc.h"
#include "utils/utils.h"
#include "utils/opengl_wrappers.h"

static const GLenum uniform_buffer_target = GL_UNIFORM_BUFFER;
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

UniformBuffer init_uniform_buffer(const GLenum usage,
	const GLchar* const block_name, const GLuint shader_using_uniform_block,
	const GLchar* const* const subvar_names, const buffer_size_t num_subvars) {

	// TODO: reset this when starting a new level
	static GLuint next_binding_point = 0;
	const GLuint binding_point = next_binding_point++;

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

	const buffer_size_t twice_num_subvars = num_subvars << 1;

	GLint* const subvar_metadata_buffer = alloc(twice_num_subvars << 1, sizeof(GLint));

	GLint
		*const subvar_sizes = subvar_metadata_buffer,
		*const subvar_byte_offsets = subvar_metadata_buffer + num_subvars,
		*const subvar_array_strides = subvar_metadata_buffer + twice_num_subvars,
		*const subvar_matrix_strides = subvar_metadata_buffer + twice_num_subvars + num_subvars;

	glGetActiveUniformsiv(shader_using_uniform_block, (GLsizei) num_subvars, subvar_indices, GL_UNIFORM_SIZE, subvar_sizes);
	glGetActiveUniformsiv(shader_using_uniform_block, (GLsizei) num_subvars, subvar_indices, GL_UNIFORM_OFFSET, subvar_byte_offsets);
	glGetActiveUniformsiv(shader_using_uniform_block, (GLsizei) num_subvars, subvar_indices, GL_UNIFORM_ARRAY_STRIDE, subvar_array_strides);
	glGetActiveUniformsiv(shader_using_uniform_block, (GLsizei) num_subvars, subvar_indices, GL_UNIFORM_MATRIX_STRIDE, subvar_matrix_strides);

	dealloc(subvar_indices);

	////////// After that, getting the block size in bytes, and defining the the uniform buffer

	GLint block_size_in_bytes;

	glGetActiveUniformBlockiv(shader_using_uniform_block,
		safely_get_uniform_block_index(shader_using_uniform_block, block_name),
		GL_UNIFORM_BLOCK_DATA_SIZE, &block_size_in_bytes);

	const GLuint buffer = init_gpu_buffer();
	use_gpu_buffer(uniform_buffer_target, buffer);
	init_gpu_buffer_data(uniform_buffer_target, 1, block_size_in_bytes, NULL, usage);
	glBindBufferBase(uniform_buffer_target, binding_point, buffer);

	////////// And finally, returning the uniform buffer

	return (UniformBuffer) {
		.id = buffer, .binding_point = binding_point,
		.gpu_memory_mapping = NULL,

		.block = {
			.size = (buffer_size_t) block_size_in_bytes,
			.name = block_name
		},

		.subvars = {
			.count = num_subvars,
			.sizes = subvar_sizes,
			.byte_offsets = subvar_byte_offsets,
			.array_strides = subvar_array_strides,
			.matrix_strides = subvar_matrix_strides,
			.names = subvar_names
		}
	};
}

void deinit_uniform_buffer(const UniformBuffer* const buffer) {
	/* The subvar metadata, which includes the sizes, byte offsets, and array and matrix strides, are all allocated
	in one block together, so freeing the sizes (which is the beginning of the block) frees all of them. */
	dealloc(buffer -> subvars.sizes);
	deinit_gpu_buffer(buffer -> id);
}

void bind_uniform_buffer_to_shader(const UniformBuffer* const buffer, const GLuint shader) {
	glUniformBlockBinding(shader, safely_get_uniform_block_index(shader, buffer -> block.name), buffer -> binding_point);
}

void enable_uniform_buffer_writing_batch(UniformBuffer* const buffer, const bool discard_prev_contents) {
	byte** const gpu_memory_mapping = &buffer -> gpu_memory_mapping;

	if (*gpu_memory_mapping != NULL)
		FAIL(InitializeShaderUniform, "Cannot enable writing batch for uniform "
			"block '%s' when the batch is already enabled", buffer -> block.name);

	*gpu_memory_mapping = init_gpu_buffer_memory_mapping(buffer -> id, uniform_buffer_target, buffer -> block.size, discard_prev_contents);
}

void disable_uniform_buffer_writing_batch(UniformBuffer* const buffer) {
	deinit_gpu_buffer_memory_mapping(uniform_buffer_target);
	buffer -> gpu_memory_mapping = NULL;
}

////////// This part concerns writing data to the uniform buffer

static void check_if_is_array(const bool expecting_array, const GLint array_length, const GLchar* const subvar_name) {
	/* Primitives are considered to have an array length of one. This
	means that arrays with a length of one are not possible. */
	if (expecting_array && array_length == 1)
		FAIL(InitializeShaderUniform, "Subvar `%s` is not an array, but "
			"writing to it as if it were one", subvar_name);
	else if (!expecting_array && array_length != 1)
		FAIL(InitializeShaderUniform, "Subvar `%s` is an array, but "
			"writing to it as if it were not one", subvar_name);
}

static void check_primitive_size(const buffer_size_t size, const GLchar* const function_name) {
	if (size == 0 || size > max_primitive_size) FAIL(InitializeShaderUniform,
		"Primitive sizes equal to 0 or larger than %zu (which is the size of %s) "
		"are not supported for `%s`. The failing size was %u", max_primitive_size,
		max_primitive_size_name, function_name, size
	);
}

static void check_matrix_size(const buffer_size_t column_size,
	const buffer_size_t num_columns, const GLchar* const function_name) {

	check_primitive_size(column_size, function_name);

	if (num_columns == 0 || num_columns > 4) FAIL(InitializeShaderUniform,
		"A matrix column count of %u when calling `%s` is not supported",
		num_columns, function_name
	);
}

static void check_array_length(const buffer_size_t expected_length, const GLint array_length,
	const GLchar* const block_name, const GLchar* const subvar_name, const GLchar* const function_name) {

	// TODO: check for an exact size match
	if (expected_length > (buffer_size_t) array_length) {
		const buffer_size_t overshoot_amount = expected_length - (buffer_size_t) array_length;

		FAIL(InitializeShaderUniform,
			"When initializing the array `%s` in `%s` for uniform block `%s`, "
				"the array length was %u item%s too long", subvar_name, function_name,
				block_name, overshoot_amount, (overshoot_amount == 1) ? "" : "s"
		);
	}
}

// If either stride is null, it will not be written to.
static void get_subvar_metadata(const UniformBuffer* const buffer, const GLchar* const subvar_name,
	byte** const gpu_memory_dest, GLint* const array_length, GLint* const array_stride, GLint* const matrix_stride) {

	byte* const gpu_memory_mapping = buffer -> gpu_memory_mapping;

	if (gpu_memory_mapping == NULL) FAIL(InitializeShaderUniform, "Cannot write subvar '%s'"
		" to uniform buffer because a writing batch is not enabled", subvar_name
	);

	const buffer_size_t num_subvars = buffer -> subvars.count;
	const GLchar* const* const subvar_names = buffer -> subvars.names;

	for (buffer_size_t i = 0; i < num_subvars; i++) {
		if (!strcmp(subvar_name, subvar_names[i])) {
			// In some cases, need the gpu buffer ptr. Never need the actual index. Also need some strides in some cases.
			*gpu_memory_dest = buffer -> gpu_memory_mapping + buffer -> subvars.byte_offsets[i];

			const bool expecting_array = array_length != NULL;
			const GLint array_length_copy = buffer -> subvars.sizes[i];
			check_if_is_array(expecting_array, array_length_copy, subvar_name);

			if (expecting_array) *array_length = array_length_copy;
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
	get_subvar_metadata(buffer, subvar_name, &dest, NULL, NULL, NULL);
	memcpy(dest, primitive, size);
}

void write_matrix_to_uniform_buffer(const UniformBuffer* const buffer, const GLchar* const subvar_name,
	const GLfloat* const matrix, const buffer_size_t column_size, const buffer_size_t num_columns) {

	check_matrix_size(column_size, num_columns, "write_matrix_to_uniform_buffer");

	byte* dest;
	GLint matrix_stride;
	get_subvar_metadata(buffer, subvar_name, &dest, NULL, NULL, &matrix_stride);

	const byte* src = (byte*) matrix;

	for (buffer_size_t i = 0; i < num_columns; i++) {
		memcpy(dest, src, column_size);
		dest += matrix_stride;
		src += column_size;
	}
}

void write_array_of_primitives_to_uniform_buffer(const UniformBuffer* const buffer,
	const GLchar* const subvar_name, const List primitives) {

	const GLchar* const function_name = "write_array_of_primitives_to_uniform_buffer";
	check_primitive_size(primitives.item_size, function_name);

	byte* dest;
	GLint array_stride, array_length;
	get_subvar_metadata(buffer, subvar_name, &dest, &array_length, &array_stride, NULL);
	check_array_length(primitives.length, array_length, buffer -> block.name, subvar_name, function_name);

	LIST_FOR_EACH(0, &primitives, primitive, _,
		memcpy(dest, primitive, primitives.item_size);
		dest += array_stride;
	);
}

void write_array_of_matrices_to_uniform_buffer(const UniformBuffer* const buffer,
	const GLchar* const subvar_name, const GLfloat* const* const matrices,
	const buffer_size_t num_matrices, const buffer_size_t column_size,
	const buffer_size_t num_columns) {

	const GLchar* const function_name = "write_matrix_to_uniform_buffer";
	check_matrix_size(column_size, num_columns, function_name);

	byte* dest;
	GLint matrix_stride, array_length;
	get_subvar_metadata(buffer, subvar_name, &dest, &array_length, NULL, &matrix_stride);
	check_array_length(num_matrices, array_length, buffer -> block.name, subvar_name, function_name);

	const byte* src = (byte*) matrices;

	for (buffer_size_t i = 0; i < num_matrices; i++) {
		for (buffer_size_t i = 0; i < num_columns; i++) {
			memcpy(dest, src, column_size);
			dest += matrix_stride;
			src += column_size;
		}
	}
}
