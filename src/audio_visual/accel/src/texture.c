#ifndef TEXTURE_C
#define TEXTURE_C

#include "headers/texture.h"
#include "headers/utils.h"

static SDL_Surface* init_blank_surface(const GLsizei width, const GLsizei height) {
	return SDL_CreateRGBSurfaceWithFormat(0, width, height, SDL_BITSPERPIXEL(SDL_PIXEL_FORMAT), SDL_PIXEL_FORMAT);
}

SDL_Surface* init_surface(const char* const path) {
	SDL_Surface* const surface = SDL_LoadBMP(path);
	if (surface == NULL) fail("open texture file", OpenImageFile);

	SDL_Surface* const converted_surface = SDL_ConvertSurfaceFormat(surface, SDL_PIXEL_FORMAT, 0);
	SDL_FreeSurface(surface);

	return converted_surface;
}

void deinit_surface(SDL_Surface* const surface) {
	SDL_FreeSurface(surface);
}

void use_texture(const GLuint texture, const GLuint shader_program, const TextureType texture_type) {
	static byte first_call = 1;
	if (first_call) {
		glActiveTexture(GL_TEXTURE0);
		first_call = 0;
	}

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

// This function assumes that the surface is locked beforehand. This is legacy code and should be removed if possible
void write_surface_to_texture(const SDL_Surface* const surface, const GLenum opengl_texture_type) {
	glTexImage2D(opengl_texture_type, 0, OPENGL_INTERNAL_PIXEL_FORMAT,
		surface -> w, surface -> h, 0, OPENGL_INPUT_PIXEL_FORMAT,
		OPENGL_COLOR_CHANNEL_TYPE, surface -> pixels);
}

// This is legacy code and should be removed if possible
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

		write_surface_to_texture(surface, TexPlain);
		glGenerateMipmap(TexPlain);
		deinit_surface(surface);
	}

	va_end(args);
	return textures;
}

static void init_still_subtextures_in_texture_set(
	const GLsizei num_still_subtextures, SDL_Surface* const rescaled_surface, va_list args) {

	for (GLsizei i = 0; i < num_still_subtextures; i++) {
		SDL_Surface* const surface = init_surface(va_arg(args, char*));
		SDL_Surface* surface_copied_to_gpu;

		if (surface -> w != rescaled_surface -> w || surface -> h != rescaled_surface -> h) {
			SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
			SDL_BlitScaled(surface, NULL, rescaled_surface, NULL);
			surface_copied_to_gpu = rescaled_surface;
		}
		else surface_copied_to_gpu = surface;

		SDL_LockSurface(surface_copied_to_gpu);
		glTexSubImage3D(TexSet, 0, 0, 0, i,
			surface_copied_to_gpu -> w, surface_copied_to_gpu -> h, 1, OPENGL_INPUT_PIXEL_FORMAT,
			OPENGL_COLOR_CHANNEL_TYPE, surface_copied_to_gpu -> pixels);

		SDL_UnlockSurface(surface_copied_to_gpu);
		deinit_surface(surface);
	}
}

static void init_animated_subtextures_in_texture_set(const GLsizei num_animated_frames,
	const GLsizei num_still_subtextures, SDL_Surface* const rescaled_surface, va_list args) {

	/////////////////
	for (GLsizei animation_frame_index = num_still_subtextures; animation_frame_index < num_animated_frames;) {
		SDL_Surface* const spritesheet_surface = init_surface(va_arg(args, char*));
		SDL_SetSurfaceBlendMode(spritesheet_surface, SDL_BLENDMODE_NONE);

		const GLsizei
			frames_across = va_arg(args, GLsizei),
			frames_down = va_arg(args, GLsizei),
			total_frames = va_arg(args, GLsizei);

		SDL_Rect spritesheet_frame_area = {
			.w = spritesheet_surface -> w / frames_across,
			.h = spritesheet_surface -> h / frames_down
		};

		for (GLsizei frame_index = 0; frame_index < total_frames; frame_index++, animation_frame_index++) {
			spritesheet_frame_area.x = (frame_index % frames_across) * spritesheet_frame_area.w;
			spritesheet_frame_area.y = (frame_index / frames_across) * spritesheet_frame_area.h;

			SDL_BlitScaled(spritesheet_surface, &spritesheet_frame_area, rescaled_surface, NULL);
			SDL_LockSurface(rescaled_surface);

			glTexSubImage3D(TexSet, 0, 0, 0, animation_frame_index,
				rescaled_surface -> w, rescaled_surface -> h, 1, OPENGL_INPUT_PIXEL_FORMAT,
				OPENGL_COLOR_CHANNEL_TYPE, rescaled_surface -> pixels);

			SDL_UnlockSurface(rescaled_surface);
		}
		deinit_surface(spritesheet_surface);
	}
}

// Unanimated sprites should go first when passed in variadically
GLuint init_texture_set(const TextureWrapMode wrap_mode, const GLsizei num_still_subtextures,
	const GLsizei num_animation_sets, const GLsizei rescale_w, const GLsizei rescale_h, ...) {

	va_list args, args_copy;
	va_start(args, rescale_h);
	va_copy(args_copy, args);

	////////// Getting number of animated frames for all animations

	GLsizei num_animated_frames = 0; // A frame is a subtexture

	for (GLsizei i = 0; i < num_still_subtextures; i++, va_arg(args_copy, char*)); // Discarding still subtexture args
	for (GLsizei i = 0; i < num_animation_sets; i++) {
		va_arg(args_copy, char*); // Discarding path, frames across, and frames down args
		va_arg(args_copy, GLsizei);
		va_arg(args_copy, GLsizei);
		num_animated_frames += va_arg(args_copy, GLsizei); // Adding num frames for one animation set
	}

	va_end(args_copy);

	////////// Defining texture and rescaled surface

	const GLsizei total_num_subtextures = num_still_subtextures + num_animated_frames;
	const GLuint texture = preinit_texture(TexSet, wrap_mode);

	glTexImage3D(TexSet, 0, OPENGL_INTERNAL_PIXEL_FORMAT, rescale_w,
		rescale_h, total_num_subtextures, 0, OPENGL_INPUT_PIXEL_FORMAT,
		OPENGL_COLOR_CHANNEL_TYPE, NULL);

	SDL_Surface* const rescaled_surface = init_blank_surface(rescale_w, rescale_h);

	////////// Filling array texture with still and animated subtextures

	init_still_subtextures_in_texture_set(num_still_subtextures, rescaled_surface, args);
	init_animated_subtextures_in_texture_set(num_animated_frames, num_still_subtextures, rescaled_surface, args);

	glGenerateMipmap(TexSet);
	deinit_surface(rescaled_surface);
	va_end(args);
	return texture;
}

#endif
