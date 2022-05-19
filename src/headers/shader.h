#ifndef SHADER_H
#define SHADER_H

#include "buffer_defs.h"

#define use_shader glUseProgram
#define deinit_shader glDeleteProgram

/* Excluded: fail_on_shader_creation_error, init_shader_from_source, read_file_contents,
get_source_for_included_file, read_and_parse_includes_for_glsl, erase_version_strings_from_dependency_list */

GLuint init_shader(const GLchar* const vertex_shader_path, const GLchar* const fragment_shader_path);

#endif
