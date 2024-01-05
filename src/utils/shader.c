#include "utils/shader.h"
#include <stdbool.h> // For `bool`
#include "utils/list.h" // For various `List`-related defs
#include "utils/failure.h" // For `FAIL`
#include "data/constants.h" // For `PRINT_SHADER_VALIDATION_LOG`
#include "utils/safe_io.h" // For `read_file_contents`
#include <string.h> // For various string utils

enum {num_sub_shaders = 3}; // Vertex, geometry, and fragment

/* This is just here because `read_and_parse_includes_for_glsl`
and `get_source_for_included_file` are mutually recursive */
static bool read_and_parse_includes_for_glsl(List* const dependency_list,
	GLchar* const sub_shader_code, const GLchar* const sub_shader_path);

//////////

static void report_shader_creation_error(
	const GLuint object,
	const GLchar* const sub_shader_path,
	const GLchar* const creation_step,
	void (*const creation_action) (const GLuint),
	void (*const log_length_getter) (const GLuint, const GLenum, GLint* const),
	void (*const log_getter) (const GLuint, const GLsizei, GLsizei* const, GLchar* const)) {

	creation_action(object);

	GLint log_length;
	log_length_getter(object, GL_INFO_LOG_LENGTH, &log_length);

	if (log_length > 0) {
		GLchar* const error_message = alloc((size_t) log_length + 1, sizeof(GLchar));
		log_getter(object, log_length, NULL, error_message);

		// No newlines in the error message!
		for (GLchar* c = error_message; *c != '\0'; c++) {
			if (*c == '\n') *c = '\0';
		}

		// Nothing else is deallocated anyways during `FAIL`, so it's fine if the `alloc` call is not freed
		FAIL(CreateShader, "Error for '%s' during %s: '%s'", sub_shader_path, creation_step, error_message);
	}
}

#ifdef PRINT_SHADER_VALIDATION_LOG

static void report_shader_validation_error(const GLuint shader, const GLchar* const shader_path) {
	glValidateProgram(shader);

	GLint log_length;
	glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &log_length);

	if (log_length > 0) {
		GLchar* const info_log = alloc((size_t) (log_length + 1), sizeof(GLchar));
		glGetProgramInfoLog(shader, log_length, NULL, info_log);
		fprintf(stderr, "Problem for shader of path '%s':\n%s\n---\n", shader_path, info_log);
		dealloc(info_log);
	}
}

#endif

/* If a sub-shader path is null, the sub-shader code corresponding
to that path will not be compiled into the final shader */
static GLuint init_shader_from_source(
	const List shader_code[num_sub_shaders],
	const GLchar* const sub_shader_paths[num_sub_shaders],
	void (*const hook_before_linking) (const GLuint shader)) {

	static const GLenum sub_shader_types[num_sub_shaders] = {GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER};

	GLuint sub_shaders[num_sub_shaders], shader = glCreateProgram();

	for (byte i = 0; i < num_sub_shaders; i++) {
		const GLchar* const sub_shader_path = sub_shader_paths[i];
		if (sub_shader_path == NULL) continue;

		const GLuint sub_shader = glCreateShader(sub_shader_types[i]);
		const List* const sub_shader_code = shader_code + i;

		glShaderSource(sub_shader, (GLsizei) sub_shader_code -> length, sub_shader_code -> data, NULL);
		report_shader_creation_error(sub_shader, sub_shader_path, "compilation", glCompileShader, glGetShaderiv, glGetShaderInfoLog);

		glAttachShader(shader, sub_shader);
		sub_shaders[i] = sub_shader;
	}

	if (hook_before_linking != NULL) hook_before_linking(shader);

	// Picking an arbitrary sub-shader for this (as long as the author recognizes what sub-shader group is failing)
	const GLchar* const vertex_sub_shader = sub_shader_paths[0];
	report_shader_creation_error(shader, vertex_sub_shader, "linking", glLinkProgram, glGetProgramiv, glGetProgramInfoLog);

	for (byte i = 0; i < num_sub_shaders; i++) {
		if (sub_shader_paths[i] == NULL) continue;

		const GLuint sub_shader = sub_shaders[i];
		glDetachShader(shader, sub_shader);
		glDeleteShader(sub_shader);
	}

	#ifdef PRINT_SHADER_VALIDATION_LOG
	report_shader_validation_error(shader, vertex_sub_shader);
	#endif

	return shader;
}

static GLchar* get_source_for_included_file(List* const dependency_list,
	const GLchar* const includer_path, const GLchar* const included_path) {

	/* When a shader includes a file, the file it includes should be relative
	to its filesystem directory. So, this function finds a new path for the included
	file, which is the concatenation of the base includer path with the included file path. */

	////////// Calculating the base path length for the included shader

	const GLchar* const last_slash_pos_in_includer_path = strrchr(includer_path, '/');

	const int base_path_length = (last_slash_pos_in_includer_path == NULL)
		? 0 // If there's no slash in the path, there's no base path
		: (last_slash_pos_in_includer_path - includer_path + 1);

	////////// Making a new string that concatenates the includer base path with the included path

	GLchar* const full_path_string_for_included = make_formatted_string("%.*s%s", base_path_length, includer_path, included_path);

	////////// Reading the included file, blanking out #versions, recursively reading its #includes, and returning the contents

	GLchar* const included_contents = read_file_contents(full_path_string_for_included);
	while (read_and_parse_includes_for_glsl(dependency_list, included_contents, full_path_string_for_included));

	dealloc(full_path_string_for_included);

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

	const GLchar* const include_directive = "#include";

	//////////

	GLchar* const include_string = strstr(sub_shader_code, include_directive);
	if (include_string == NULL) return false;

	////////// Skipping newlines and tabs

	GLchar* after_include_string = include_string + strlen(include_directive);
	for (GLchar c = *after_include_string; c == ' ' || c == '\t'; c = *(++after_include_string));
	if (*after_include_string != '\"') goto error; // Other character or EOF

	////////// Finding a righthand quote, failing if a newline is reached

	bool found_right_quote = false;

	GLchar* curr_path_substring = after_include_string + 1;
	for (GLchar c = *curr_path_substring; !found_right_quote; c = *(++curr_path_substring)) {
		switch (c) {
			case '\0': case '\r': case '\n':
				goto error;
			case '\"':
				// Putting a null terminator here so that the path can be read as its own string
				*curr_path_substring = '\0';
				found_right_quote = true;
		}
	}

	////////// Fetching the included code

	const GLchar* const included_path = after_include_string + 1;
	GLchar* const included_code = get_source_for_included_file(dependency_list, sub_shader_path, included_path);
	push_ptr_to_list(dependency_list, &included_code); // The included code is freed by `init_shader`

	//////////

	// Replacing the #include region with whitespace (thus removing the null terminator)
	memset(include_string, ' ', (size_t) (curr_path_substring - include_string));

	return true;

	error: FAIL(ParseIncludeDirectiveInShader, "Path string "
			"expected after #include for '%s'", sub_shader_path);
}

static void erase_version_strings_from_dependency_list(const List* const dependency_list) {
	const GLchar *const base_version_string = "#version", *const full_version_string = "#version ___ core";
	const GLsizei full_version_string_length = strlen(full_version_string);

	const void* const first_dependency = dependency_list -> data;

	LIST_FOR_EACH(dependency_list, GLchar*, dependency_ref,
		/* Not erasing the version string from the first one
		because it's the only one that should keep #version in it */
		if (dependency_ref == first_dependency) continue;

		// TODO: expect no version string in each shader, and prepend it manually
		GLchar* const version_string_pos = strstr(*dependency_ref, base_version_string);
		if (version_string_pos != NULL) memset(version_string_pos, ' ', full_version_string_length);
		// TODO: fail here if no version string, printing the included file path
	);
}

GLuint init_shader(
	const GLchar* const vertex_shader_path,
	const GLchar* const geo_shader_path,
	const GLchar* const fragment_shader_path,
	void (*const hook_before_linking) (const GLuint shader)) {

	const GLchar* const paths[num_sub_shaders] = {vertex_shader_path, geo_shader_path, fragment_shader_path};
	List dependency_lists[num_sub_shaders];

	for (byte i = 0; i < num_sub_shaders; i++) {
		const GLchar* const path = paths[i];
		if (path == NULL) continue;

		List* const dependency_list = dependency_lists + i;
		*dependency_list = init_list(1, GLchar*);

		GLchar* const code = read_file_contents(path);

		// `read_and_parse_includes_for_glsl` blanks out #include lines and recursively adds to the dependency list
		while (read_and_parse_includes_for_glsl(dependency_list, code, path));

		push_ptr_to_list(dependency_list, &code);

		// This blanks out `#version` lines for all included files
		erase_version_strings_from_dependency_list(dependency_list);
	}

	const GLuint shader = init_shader_from_source(dependency_lists, paths, hook_before_linking);

	for (byte i = 0; i < num_sub_shaders; i++) {
		if (paths[i] == NULL) continue;

		const List* const dependency_list = dependency_lists + i;
		LIST_FOR_EACH(dependency_list, GLchar*, dependency_ref, dealloc(*dependency_ref););
		deinit_list(*dependency_list);
	}

	return shader;
}
