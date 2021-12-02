#ifndef TEXTURE_SET_C
#define TEXTURE_SET_C

#include "headers/texture_set.h"

// Texture path
GLuint init_texture_set(const GLsizei subtex_width, const GLsizei subtex_height,
	const GLsizei num_textures, const GLint texture_wrap_mode, ...) {

	GLuint ts;
	glGenTextures(1, &ts);
	glBindTexture(GL_TEXTURE_2D_ARRAY, ts);

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, OPENGL_TEX_MAG_FILTER);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, OPENGL_TEX_MIN_FILTER);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, texture_wrap_mode);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, texture_wrap_mode);

	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, OPENGL_INTERNAL_PIXEL_FORMAT,
		subtex_width, subtex_height, num_textures,
		0, OPENGL_INPUT_PIXEL_FORMAT, OPENGL_COLOR_CHANNEL_TYPE, NULL);
	
	va_list args;
	va_start(args, texture_wrap_mode);
	for (GLsizei i = 0; i < num_textures; i++) {
		const char* const path = va_arg(args, char*);
		SDL_Surface* const surface = init_surface(path);

		if (surface -> w != subtex_width || surface -> h != subtex_height) {
			fprintf(stderr, "Expected image '%s' to have a size of {%d, %d}\n", path, subtex_width, subtex_height);
			fail("make a texture set with a mismatching image size", TextureSetMismatchingImageSize);
		}

		glTexSubImage3D(
			GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, subtex_width, subtex_height, 1,
			OPENGL_INPUT_PIXEL_FORMAT, OPENGL_COLOR_CHANNEL_TYPE, surface -> pixels);
		
		deinit_surface(surface);
	}

	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
	va_end(args);

	return ts;
}

#endif
