#ifndef SHADER_H
#define SHADER_H

#define use_shader glUseProgram
#define deinit_shader glDeleteProgram

// Excluded: fail_on_shader_creation_error, init_shader_from_source, read_file_contents

GLuint init_shader(const GLchar* const vertex_shader_path, const GLchar* const fragment_shader_path);

#endif
