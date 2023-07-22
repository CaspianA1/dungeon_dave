#ifndef OPENGL_WRAPPERS_H
#define OPENGL_WRAPPERS_H

#include "glad/glad.h" // For OpenGL types
#include "utils/typedefs.h" // For various typedefs
#include "utils/failure.h" // For `FAIL`

static const GLenum framebuffer_target = GL_DRAW_FRAMEBUFFER;

////////// Some init/use/deinit functions

#define init_gpu_buffer_data(target, num_items, item_size, data, access) glBufferData((target), (num_items) * (item_size), (data), (access))
#define init_vertex_buffer_data(num_items, item_size, data, access) init_gpu_buffer_data(GL_ARRAY_BUFFER, num_items, item_size, data, access)

#define reinit_gpu_buffer_data(target, num_items, item_size, data) glBufferSubData((target), 0, (num_items) * (item_size), (data))
#define reinit_vertex_buffer_data(num_items, item_size, data) reinit_gpu_buffer_data(GL_ARRAY_BUFFER, num_items, item_size, data)

#define init_texture_mipmap glGenerateMipmap
#define deinit_surface SDL_FreeSurface

#define use_shader glUseProgram
#define deinit_shader glDeleteProgram

#define init_vertex_buffer_memory_mapping(buffer, num_bytes, discard_prev_contents)\
	init_gpu_buffer_memory_mapping(buffer, GL_ARRAY_BUFFER, num_bytes, discard_prev_contents)

#define deinit_gpu_buffer_memory_mapping glUnmapBuffer
#define deinit_vertex_buffer_memory_mapping() deinit_gpu_buffer_memory_mapping(GL_ARRAY_BUFFER)

#define GL_USE(suffix) glBind##suffix
	#define use_gpu_buffer GL_USE(Buffer)
	#define use_vertex_buffer(vertex_buffer) use_gpu_buffer(GL_ARRAY_BUFFER, (vertex_buffer))
	#define use_vertex_spec GL_USE(VertexArray)
	#define use_texture GL_USE(Texture)
	#define use_framebuffer GL_USE(Framebuffer)

#define GL_DEINIT(suffix, object) glDelete##suffix##s(1, &(object))
	#define deinit_gpu_buffer(buffer) GL_DEINIT(Buffer, buffer)
	#define deinit_vertex_spec(vertex_spec) GL_DEINIT(VertexArray, vertex_spec)
	#define deinit_texture(texture) GL_DEINIT(Texture, texture)
	#define deinit_framebuffer(framebuffer) GL_DEINIT(Framebuffer, framebuffer)

#define draw_primitives(mode, num_primitives) glDrawArrays(mode, 0, num_primitives)
#define draw_instances(mode, num_entities, num_instances) glDrawArraysInstanced(mode, 0, num_entities, num_instances)

////////// Some uniform setters

#define INIT_UNIFORM(name, shader) name##_id = safely_get_uniform((shader), #name)

#define INIT_UNIFORM_VALUE(name, shader, type_prefix, ...)\
	glUniform##type_prefix(safely_get_uniform((shader), #name), __VA_ARGS__)

#define UPDATE_UNIFORM(name, type_prefix, ...) glUniform##type_prefix(name##_id, __VA_ARGS__)

////////// Some render state setters

#define GENERIC_WITH_RENDER_STATE(setter, unsetter, state, inverse_state, ...) do {\
	setter((state)); __VA_ARGS__ unsetter((inverse_state));\
} while (false)

#define WITH_BINARY_RENDER_STATE(state, ...) GENERIC_WITH_RENDER_STATE(glEnable, glDisable, state, state, __VA_ARGS__)
#define WITHOUT_BINARY_RENDER_STATE(state, ...) GENERIC_WITH_RENDER_STATE(glDisable, glEnable, state, state, __VA_ARGS__)
#define WITH_RENDER_STATE(setter, state, inverse_state, ...) GENERIC_WITH_RENDER_STATE(setter, setter, state, inverse_state, __VA_ARGS__)
#define WITHOUT_VERTEX_SPEC_INDEX(i, ...) GENERIC_WITH_RENDER_STATE(glDisableVertexAttribArray, glEnableVertexAttribArray, i, i, __VA_ARGS__)

////////// Some inline functions

static inline GLint safely_get_uniform(const GLuint shader, const GLchar* const name) {
	const GLint id = glGetUniformLocation(shader, name);
	if (id == -1) FAIL(InitializeShaderUniform, "Uniform with the name of '%s' was not found in shader", name);
	return id;
}

#define GL_INIT_FN(gl_suffix, my_suffix)\
	static inline GLuint init_##my_suffix(void) {\
		GLuint object; glGen##gl_suffix##s(1, &object); return object;\
	}

GL_INIT_FN(Buffer, gpu_buffer)
GL_INIT_FN(VertexArray, vertex_spec)
GL_INIT_FN(Framebuffer, framebuffer)

#undef GL_INIT_FN

static inline void define_vertex_spec_index(
	const bool is_instanced, const bool treat_vertices_as_floats,
	const byte index, const byte num_components, const buffer_size_t stride,
	const buffer_size_t initial_offset, const GLenum typename) {

	glEnableVertexAttribArray(index);
	if (is_instanced) glVertexAttribDivisor(index, 1);

	const GLsizei cast_stride = (GLsizei) stride;
	const void* const cast_initial_offset = (void*) (size_t) initial_offset;

	if (treat_vertices_as_floats) glVertexAttribPointer(index, num_components, typename, GL_FALSE, cast_stride, cast_initial_offset);
	else glVertexAttribIPointer(index, num_components, typename, cast_stride, cast_initial_offset);
}

// This is only for writing, not reading. TODO: possibly add `GL_MAP_UNSYNCHRONIZED_BIT`, if possible? Test on Chromebook.
static inline void* init_gpu_buffer_memory_mapping(const GLuint buffer, const GLenum target, const GLsizeiptr num_bytes, const bool discard_prev_contents) {
	use_gpu_buffer(target, buffer);
	const GLbitfield range_invalidation_flag = discard_prev_contents ? GL_MAP_INVALIDATE_RANGE_BIT : 0;
	return glMapBufferRange(target, 0, num_bytes, GL_MAP_WRITE_BIT | range_invalidation_flag);
}

static inline void check_framebuffer_completeness(void) {
	const GLchar* status_string;

	switch (glCheckFramebufferStatus(framebuffer_target)) {
		case GL_FRAMEBUFFER_COMPLETE:
			status_string = NULL;
			break;

		#define COMPLETENESS_CASE(status) case GL_##status: status_string = #status; break

		COMPLETENESS_CASE(FRAMEBUFFER_UNDEFINED);
		COMPLETENESS_CASE(FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
		COMPLETENESS_CASE(FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
		COMPLETENESS_CASE(FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER);
		COMPLETENESS_CASE(FRAMEBUFFER_INCOMPLETE_READ_BUFFER);
		COMPLETENESS_CASE(FRAMEBUFFER_UNSUPPORTED);
		COMPLETENESS_CASE(FRAMEBUFFER_INCOMPLETE_MULTISAMPLE);
		COMPLETENESS_CASE(FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS);

		#undef COMPLETENESS_CASE

		default: status_string = "Unknown framebuffer error";
	}

	if (status_string != NULL)
		FAIL(CreateFramebuffer, "Could not create a framebuffer for this reason: '%s'", status_string);
}

static inline const GLchar* get_GL_error(void) {
	switch (glGetError()) {
		#define ERROR_CASE(error) case GL_##error: return #error;

		ERROR_CASE(NO_ERROR);
		ERROR_CASE(INVALID_ENUM);
		ERROR_CASE(INVALID_VALUE);
		ERROR_CASE(INVALID_OPERATION);
		ERROR_CASE(INVALID_FRAMEBUFFER_OPERATION);
		ERROR_CASE(OUT_OF_MEMORY);

		#undef ERROR_CASE
	}
	return "Unknown error";
}

#endif
