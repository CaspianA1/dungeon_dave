#ifndef SHADER_C
#define SHADER_C

#include "headers/buffer_defs.h"

typedef enum {
	CompileVertexShader,
	CompileFragmentShader,
	LinkShaders
} ShaderCompilationStep;

//////////

static void fail_on_shader_creation_error(const GLuint object_id,
	const ShaderCompilationStep compilation_step,
	void (*const creation_action) (const GLuint),
	void (*const log_length_getter) (const GLuint, const GLenum, GLint* const),
	void (*const log_getter) (const GLuint, const GLsizei, GLsizei* const, GLchar* const)) {

	creation_action(object_id);

	GLint log_length;
	log_length_getter(object_id, GL_INFO_LOG_LENGTH, &log_length);

	if (log_length > 0) {
		GLchar* const error_message = malloc((size_t) log_length + 1);
		log_getter(object_id, log_length, NULL, error_message);

		const byte compilation_step_id = (byte) compilation_step + 1;  // `fail` not used for this since the error message must be freed

        // TODO: use FAIL
		fprintf(stderr, "Shader creation step #%d - %s", compilation_step_id, error_message);
		free(error_message);
		exit(compilation_step_id);
	}
}

GLuint init_shader(const GLchar* const vertex_shader_code, const GLchar* const fragment_shader_code) {
	// In this, a sub-shader is a part of the big shader, like a vertex or fragment shader.
	const GLchar* const sub_shader_code[2] = {vertex_shader_code, fragment_shader_code};
	const GLenum sub_shader_types[2] = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
	GLuint sub_shaders[2], shader = glCreateProgram();

	for (ShaderCompilationStep step = CompileVertexShader; step < LinkShaders; step++) {
		const GLuint sub_shader = glCreateShader(sub_shader_types[step]);
		glShaderSource(sub_shader, 1, sub_shader_code + step, NULL);

		fail_on_shader_creation_error(sub_shader, step,
			glCompileShader, glGetShaderiv, glGetShaderInfoLog);

		glAttachShader(shader, sub_shader);

		sub_shaders[step] = sub_shader;
	}

	fail_on_shader_creation_error(shader, LinkShaders,
		glLinkProgram, glGetProgramiv, glGetProgramInfoLog);

	for (ShaderCompilationStep step = CompileVertexShader; step < LinkShaders; step++) {
		const GLuint sub_shader = sub_shaders[step];
		glDetachShader(shader, sub_shader);
		glDeleteShader(sub_shader);
	}

	return shader;
}

#endif
