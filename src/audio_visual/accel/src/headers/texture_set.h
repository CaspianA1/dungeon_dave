#ifndef TEXTURE_SET_H
#define TEXTURE_SET_H

#include "utils.h"

GLuint init_texture_set(const GLsizei subtex_width, const GLsizei subtex_height,
	const GLsizei num_textures, const GLint texture_wrap_mode, ...);

#endif

