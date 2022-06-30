#ifndef SHADER_C
#define SHADER_C

#include "headers/shader.h"
#include "headers/utils.h"
#include "headers/list.h"

typedef enum {
	CompileVertexShader,
	CompileGeoShader,
	CompileFragmentShader,
	LinkShaders
} ShaderCompilationStep;

enum {num_sub_shaders = 3};

/* This is just here because `read_and_parse_includes_for_glsl`
and `get_source_for_included_file` are mutually recursive */
static bool read_and_parse_includes_for_glsl(List* const dependency_list,
	GLchar* const sub_shader_code, const GLchar* const sub_shader_path);

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

		#define STRING_CASE(enum_name, string) case enum_name: compilation_step_string = string; break

		const GLchar* compilation_step_string;

		switch (compilation_step) {
			STRING_CASE(CompileVertexShader, "vertex shader compilation");
			STRING_CASE(CompileGeoShader, "geometry shader compilation");
			STRING_CASE(CompileFragmentShader, "fragment shader compilation");
			STRING_CASE(LinkShaders, "linking");
		}

		#undef STRING_CASE

		// Nothing else is freed anyways during `FAIL`, so it's fine if the `malloc` call is not freed
		FAIL(CreateShader, "Error during %s: '%s'", compilation_step_string, error_message);
	}
}

static GLuint init_shader_from_source(const List shader_code[num_sub_shaders], const GLchar* const sub_shader_paths[num_sub_shaders]) {
	const GLenum sub_shader_types[num_sub_shaders] = {GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER};
	GLuint sub_shaders[num_sub_shaders], shader = glCreateProgram();

	for (ShaderCompilationStep step = CompileVertexShader; step < LinkShaders; step++) {
		if (sub_shader_paths[step] == NULL) continue;

		const GLuint sub_shader = glCreateShader(sub_shader_types[step]);

		const List* const sub_shader_code = shader_code + step;
		glShaderSource(sub_shader, (GLsizei) sub_shader_code -> length, sub_shader_code -> data, NULL);

		fail_on_shader_creation_error(sub_shader, step,
			glCompileShader, glGetShaderiv, glGetShaderInfoLog);

		glAttachShader(shader, sub_shader);

		sub_shaders[step] = sub_shader;
	}

	fail_on_shader_creation_error(shader, LinkShaders,
		glLinkProgram, glGetProgramiv, glGetProgramInfoLog);

	for (ShaderCompilationStep step = CompileVertexShader; step < LinkShaders; step++) {
		if (sub_shader_paths[step] == NULL) continue;

		const GLuint sub_shader = sub_shaders[step];
		glDetachShader(shader, sub_shader);
		glDeleteShader(sub_shader);
	}

	return shader;
}

static char* read_file_contents(const char* const path) {
	FILE* const file = open_file_safely(path, "r");

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

static GLchar* get_source_for_included_file(List* const dependency_list,
	const GLchar* const includer_path, const GLchar* const included_path) {

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

	const size_t included_full_path_string_length = base_path_length + strlen(included_path);

	// One more character for the null terminator
	GLchar* const full_path_string_for_included = malloc(included_full_path_string_length + 1);

	memcpy(full_path_string_for_included, includer_path, base_path_length);
	strcpy(full_path_string_for_included + base_path_length, included_path);

	////////// Reading the included file, blanking out #versions, recursively reading its #includes, and returning the contents

	GLchar* const included_contents = read_file_contents(full_path_string_for_included);
	while (read_and_parse_includes_for_glsl(dependency_list, included_contents, full_path_string_for_included));
	free(full_path_string_for_included);

	return included_contents;
}

// Returns if an include snippet was found
static bool read_and_parse_includes_for_glsl(List* const dependency_list,
	GLchar* const sub_shader_code, const GLchar* const sub_shader_path) {

	/*
	#include specification:

	1. String that equals #include
	2. Arbitrary whitespace, excluding newline characters (or no whitespace)
	3. Double quotation mark
	4. Path string without double quotation mark, with no newline characters
	5. Double quotation mark
	6. Newline or EOF

	To handle later:
	1. Arbitrary whitespace possible between hashtag and #include
	2. Ignore #include directives in single or multi-line comments

	Other things to do for this:
	1. Detect dependency cycles
	2. Perhaps handle #defines
	3. Correct line numbers for errors when including files
	*/

	#define NO_PATH_STRING_ERROR() FAIL(ParseIncludeDirectiveInShader,\
		"Path string expected after #include for '%s'", sub_shader_path)

	const GLchar *const include_directive = "#include";

	//////////

	GLchar* const include_string = strstr(sub_shader_code, include_directive);
	if (include_string == NULL) return false;

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
				// Putting a null terminator here so that the path can be read as its own string
				*curr_path_substring = '\0';
				found_right_quote = true;
		}
	}

	#undef NO_PATH_STRING_ERROR

	////////// Fetching the included code, and replacing the #include region with whitespace

	GLchar* const included_code = get_source_for_included_file(dependency_list, sub_shader_path, after_include_string + 1);
	push_ptr_to_list(dependency_list, &included_code); // The included code is freed by `init_shader`

	memset(include_string, ' ', (size_t) (curr_path_substring - include_string));

	return true;
}

static void erase_version_strings_from_dependency_list(const List* const dependency_list) {
	const GLchar *const base_version_string = "#version", *const full_version_string = "#version ___ core";
	const GLsizei full_version_string_length = strlen(full_version_string);

	// Not erasing the version string from the first one because it's the only one that should keep #version in it
	LIST_FOR_EACH(1, dependency_list, untyped_dependency, _,
		const GLchar* const dependency = *((GLchar**) untyped_dependency);
		GLchar* const version_string_pos = strstr(dependency, base_version_string);
		if (version_string_pos != NULL) memset(version_string_pos, ' ', full_version_string_length);
	);
}

GLuint init_shader(
	const GLchar* const vertex_shader_path,
	const GLchar* const geo_shader_path,
	const GLchar* const fragment_shader_path) {

	const GLchar* const paths[num_sub_shaders] = {vertex_shader_path, geo_shader_path, fragment_shader_path};

	List dependency_lists[num_sub_shaders];

	for (byte i = 0; i < num_sub_shaders; i++) {
		const GLchar* const path = paths[i];
		if (path == NULL) continue;

		List* const dependency_list = dependency_lists + i;
		*dependency_list = init_list(1, GLchar*);

		GLchar* const code = read_file_contents(path);

		// `get_include_snippet_in_glsl_code` blanks out #include lines and recursively adds to the dependency list
		while (read_and_parse_includes_for_glsl(dependency_list, code, path));

		push_ptr_to_list(dependency_list, &code);

		// This blanks out #version lines for all included files
		erase_version_strings_from_dependency_list(dependency_list);
	}

	const GLuint shader = init_shader_from_source(dependency_lists, paths);

	for (byte i = 0; i < num_sub_shaders; i++) {
		if (paths[i] == NULL) continue;

		const List* const dependency_list = dependency_lists + i;
		LIST_FOR_EACH(0, dependency_list, dependency_code, _, free(*(GLchar**) dependency_code););
		deinit_list(*dependency_list);
	}

	return shader;
}

#endif
