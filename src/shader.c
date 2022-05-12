#ifndef SHADER_C
#define SHADER_C

#include "headers/utils.h"

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

		// No newlines in the error message!
		for (GLchar* c = error_message; *c != '\0'; c++) {
			if (*c == '\n') *c = '\0';
		}

		const GLchar* compilation_step_string;
		switch (compilation_step) {
			case CompileVertexShader: compilation_step_string = "vertex shader compilation"; break;
			case CompileFragmentShader: compilation_step_string = "fragment shader compilation"; break;
			case LinkShaders: compilation_step_string = "linking"; break;
		}

		// Nothing else is freed anyways during `FAIL`, so it's fine if the `malloc` call is not freed
		FAIL(CreateShader, "Error during %s: \"%s\"", compilation_step_string, error_message);
	}
}

static GLuint init_shader_from_source(const GLchar* const vertex_shader_code, const GLchar* const fragment_shader_code) {
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

static char* read_file_contents(const char* const path) {
	FILE* const file = fopen(path, "r");
	if (file == NULL) FAIL(OpenFile, "could not open a file with the path of '%s'", path);

	fseek(file, 0l, SEEK_END); // Set file position to end
	const size_t num_bytes = (size_t) ftell(file);
	fseek(file, 0l, SEEK_SET); // Rewind file position

	char* const data = malloc(num_bytes + 1l);
	fread(data, num_bytes, 1, file); // Read file bytes
	data[num_bytes] = '\0';

	fclose(file);

	return data;
}

GLuint init_shader(const GLchar* const vertex_shader_path, const GLchar* const fragment_shader_path) {
	// TODO: support an #include mechanism for shader code

	GLchar
		*const vertex_shader_code = read_file_contents(vertex_shader_path),
		*const fragment_shader_code = read_file_contents(fragment_shader_path);

	const GLuint shader = init_shader_from_source(vertex_shader_code, fragment_shader_code);

	free(vertex_shader_code);
	free(fragment_shader_code);

	return shader;
}


#endif
