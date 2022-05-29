#ifndef SHADER_H
#define SHADER_H

#include "buffer_defs.h"

#define use_shader glUseProgram
#define deinit_shader glDeleteProgram

/* Excluded: fail_on_shader_creation_error, init_shader_from_source, read_file_contents,
get_source_for_included_file, read_and_parse_includes_for_glsl, erase_version_strings_from_dependency_list */

/* Null paths can be passed to this. If a path
is null, that stage of the shader isn't compiled. */
GLuint init_shader(
	const GLchar* const vertex_shader_path,
	const GLchar* const geo_shader_path,
	const GLchar* const fragment_shader_path);

#endif
