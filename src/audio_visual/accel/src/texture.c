#ifndef TEXTURE_C
#define TEXTURE_C

#include "headers/texture.h"
#include "headers/utils.h"

SDL_Surface* init_surface(const char* const path) {
	SDL_Surface* const surface = SDL_LoadBMP(path);
	if (surface == NULL) fail("open texture file", OpenImageFile);

	SDL_Surface* const converted_surface = SDL_ConvertSurfaceFormat(surface, SDL_PIXEL_FORMAT, 0);
	SDL_FreeSurface(surface);
	SDL_LockSurface(converted_surface);

	return converted_surface;
}

void deinit_surface(SDL_Surface* const surface) {
	SDL_UnlockSurface(surface);
	SDL_FreeSurface(surface);
}

void use_texture(const GLuint texture, const GLuint shader_program, const TextureType texture_type) {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(texture_type, texture); // Set the current bound texture

	const GLuint texture_sampler = glGetUniformLocation(shader_program, "texture_sampler");
	glUniform1i(texture_sampler, 0); // Make the sampler read from texture unit 0
}

GLuint preinit_texture(const TextureType texture_type, const TextureWrapMode wrap_mode) {
	GLuint t;
	glGenTextures(1, &t);
	glBindTexture(texture_type, t);

	glTexParameteri(texture_type, GL_TEXTURE_MAG_FILTER, OPENGL_TEX_MAG_FILTER);
	glTexParameteri(texture_type, GL_TEXTURE_MIN_FILTER,
		(texture_type == TexSkybox) ? OPENGL_SKYBOX_TEX_MIN_FILTER : OPENGL_TEX_MIN_FILTER);

	glTexParameteri(texture_type, GL_TEXTURE_WRAP_S, wrap_mode);
	glTexParameteri(texture_type, GL_TEXTURE_WRAP_T, wrap_mode);
	if (texture_type == TexSkybox)
		glTexParameteri(texture_type, GL_TEXTURE_WRAP_R, wrap_mode);
	else {
		#ifdef ENABLE_ANISOTROPIC_FILTERING
		float aniso;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
		glTexParameterf(texture_type, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
		#endif
	}
	
	return t;
}

// This function assumes that the surface is locked beforehand
void write_surface_to_texture(const SDL_Surface* const surface, const GLenum opengl_texture_type) {
	glTexImage2D(opengl_texture_type, 0, OPENGL_INTERNAL_PIXEL_FORMAT,
		surface -> w, surface -> h, 0, OPENGL_INPUT_PIXEL_FORMAT,
		OPENGL_COLOR_CHANNEL_TYPE, surface -> pixels);
}

GLuint* init_plain_textures(const GLsizei num_textures, ...) {
	va_list args;
	va_start(args, num_textures);

	GLuint* const textures = malloc(num_textures * sizeof(GLuint));
	glGenTextures(num_textures, textures);

	for (int i = 0; i < num_textures; i++) {
		const char* const surface_path = va_arg(args, char*);
		const TextureWrapMode wrap_mode = va_arg(args, TextureWrapMode);

		textures[i] = preinit_texture(TexPlain, wrap_mode);
		SDL_Surface* const surface = init_surface(surface_path);

		write_surface_to_texture(surface, GL_TEXTURE_2D);
		glGenerateMipmap(GL_TEXTURE_2D);
		deinit_surface(surface);
	}

	va_end(args);
	return textures;
}

// Param: Texture path
GLuint init_texture_set(const TextureWrapMode wrap_mode,
	const GLsizei subtex_width, const GLsizei subtex_height, const GLsizei num_textures, ...) {

	const GLuint ts = preinit_texture(TexSet, wrap_mode);

	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, OPENGL_INTERNAL_PIXEL_FORMAT,
		subtex_width, subtex_height, num_textures,
		0, OPENGL_INPUT_PIXEL_FORMAT, OPENGL_COLOR_CHANNEL_TYPE, NULL);
	
	SDL_Surface* const rescaled_surface = SDL_CreateRGBSurfaceWithFormat(0,
		subtex_width, subtex_height, SDL_BITSPERPIXEL(SDL_PIXEL_FORMAT), SDL_PIXEL_FORMAT);
	
	va_list args;
	va_start(args, num_textures);
	for (GLsizei i = 0; i < num_textures; i++) {
		const char* const path = va_arg(args, char*);
		SDL_Surface* const surface = init_surface(path);

		const SDL_Surface* src_surface;

		if (surface -> w != subtex_width || surface -> h != subtex_height) {
			SDL_UnlockSurface(rescaled_surface);
			SDL_UnlockSurface(surface);
			SDL_SoftStretchLinear(surface, NULL, rescaled_surface, NULL);
			SDL_LockSurface(surface);
			SDL_LockSurface(rescaled_surface);

			src_surface = rescaled_surface;
		}
		else src_surface = surface;

		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, subtex_width, subtex_height, 1,
			OPENGL_INPUT_PIXEL_FORMAT, OPENGL_COLOR_CHANNEL_TYPE, src_surface -> pixels);

		deinit_surface(surface);
	}

	deinit_surface(rescaled_surface);
	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
	va_end(args);

	return ts;
}

#endif
