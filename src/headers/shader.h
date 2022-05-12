#ifndef SHADER_H
#define SHADER_H

#define use_shader glUseProgram
#define deinit_shader glDeleteProgram

// Excluded: fail_on_shader_creation_error

GLuint init_shader_from_source(const GLchar* const vertex_shader_code, const GLchar* const fragment_shader_code);

#endif
