#ifndef SHADER_H
#define SHADER_H

#include "glad/glad.h" // For OpenGL defs

/* Excluded: report_shader_creation_error,
report_shader_validation_error, init_shader_from_source, get_source_for_included_file,
read_and_parse_includes_for_glsl, erase_version_strings_from_dependency_list */

/* Null paths can be passed to this. If a path
is null, that stage of the shader isn't compiled.

TODO: make a fn called `init_shader_with_options`, and make
`init_shader` only take the vertex and fragment shader paths. */
GLuint init_shader(
	const GLchar* const vertex_shader_path,
	const GLchar* const geo_shader_path,
	const GLchar* const fragment_shader_path,
	void (*const hook_before_linking) (const GLuint shader));

#endif
