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
		FAIL(CreateShader, "Error during %s: '%s'", compilation_step_string, error_message);
	}
}

static GLuint init_shader_from_source(
	const GLchar* const vertex_shader_code, const GLchar* const fragment_shader_code) {

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

	 /* (TODO) Possible bug: if `ftell` fails, `num_bytes` will
	 underflow, and too much data will be allocated. */

	fseek(file, 0l, SEEK_END); // Set file position to end
	const size_t num_bytes = (size_t) ftell(file);
	fseek(file, 0l, SEEK_SET); // Rewind file position

	char* const data = malloc(num_bytes + 1l);
	fread(data, num_bytes, 1, file); // Read file bytes
	data[num_bytes] = '\0';

	fclose(file);

	return data;
}

static void get_source_for_included_file(const GLchar* const includer_path, const GLchar* const included_path) {
	/* When a shader includes a file, the file it includes
	should be relative to its filesystem directory. So,
	this function finds a new path for the included file,
	which is the concatenation of the base includer path
	with the included file path. */

	////////// Calculating the base path length for the included shader

	const GLchar* const last_slash_pos_in_includer_path = strrchr(includer_path, '/');

	const size_t base_path_length = (last_slash_pos_in_includer_path == NULL)
		? 0 // If there's no slash in the path, there's no base path
		: (size_t) (last_slash_pos_in_includer_path - includer_path + 1);

	////////// Allocating a new string that concatenates the includer base path with the included path

	const size_t included_path_length = strlen(included_path);
	const size_t included_full_path_string_length = base_path_length + included_path_length;

	// One more character for the null terminator
	GLchar* const path_string_for_included = malloc(included_full_path_string_length + 1);

	memcpy(path_string_for_included, includer_path, base_path_length);
	memcpy(path_string_for_included + base_path_length, included_path, included_path_length);

	path_string_for_included[included_full_path_string_length] = '\0';

	//////////

	GLchar* const contents = read_file_contents(path_string_for_included);

	DEBUG(path_string_for_included, s);
	DEBUG(contents, s);

	free(contents);
	free(path_string_for_included);
}

static void get_include_snippet_in_glsl_code(GLchar* const sub_shader_code, const GLchar* const sub_shader_path) {
	/*
	#include specification:

	1. String that equals #include
	2. Arbitrary whitespace, excluding newline characters (or no whitespace)
	3. Double quotation mark
	4. Path string without double quotation mark, with no newline characters
	5. Double quotation mark
	6. Newline or EOF

	To handle later:
	1. Arbitrary whitespace possible between hashtag and `include
	2. Ignore #include directives in single or multi-line comments

	Other things to do for this:
	1. After extracting the path, maintain a source list, or concatenate the new file contents with the old via a realloc + strcat
	2. For a given file, extract all #includes
	3. Handles #includes recursively
	*/

	#define NO_PATH_STRING_ERROR() FAIL(ParseIncludeDirectiveInShader,\
		"Path string expected after #include for '%s'", sub_shader_path)

	const GLchar *const include_directive = "#include";

	//////////

	GLchar* const include_string = strstr(sub_shader_code, include_directive);
	if (include_string == NULL) return;

	////////// Skipping newlines and tabs

	GLchar* after_include_string = include_string + strlen(include_directive);
	for (GLchar c = *after_include_string; c == ' ' || c == '\t'; c = *(++after_include_string));
	if (*after_include_string != '\"') NO_PATH_STRING_ERROR(); // Other character or EOF

	////////// Finding a righthand quote, failing if a newline is reached

	bool found_right_quote = false;

	GLchar* curr_path_substring = after_include_string + 1;
	for (GLchar c = *curr_path_substring; !found_right_quote; c = *(++curr_path_substring)) {
		switch (c) {
			case '\0': case '\r': case '\n':
				NO_PATH_STRING_ERROR();
				break;

			case '\"':
				/* Putting a null terminator here so that
				the path can be read as its own string */
				*curr_path_substring = '\0';
				found_right_quote = true;
				break;
		}
	}

	////////// Fetching the included code, and replacing the #include region with whitespace

	get_source_for_included_file(sub_shader_path, after_include_string + 1);
	memset(include_string, ' ', (size_t) (curr_path_substring - include_string));
	#undef NO_PATH_STRING_ERROR
}

GLuint init_shader(const GLchar* const vertex_shader_path, const GLchar* const fragment_shader_path) {
	// TODO: support an #include mechanism for shader code

	const struct {GLchar *const vertex, *const fragment;} sub_shaders = {
		read_file_contents(vertex_shader_path), read_file_contents(fragment_shader_path)
	};

	// TODO: read all snippets
	get_include_snippet_in_glsl_code(sub_shaders.vertex, vertex_shader_path);
	get_include_snippet_in_glsl_code(sub_shaders.fragment, fragment_shader_path);

	const GLuint shader = init_shader_from_source(sub_shaders.vertex, sub_shaders.fragment);

	free(sub_shaders.vertex);
	free(sub_shaders.fragment);

	return shader;
}

#endif
