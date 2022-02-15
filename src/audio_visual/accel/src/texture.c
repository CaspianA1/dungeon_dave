#ifndef TEXTURE_C
#define TEXTURE_C

#include "headers/texture.h"

SDL_Surface* init_blank_surface(const GLsizei width, const GLsizei height) {
	return SDL_CreateRGBSurfaceWithFormat(0, width, height, SDL_BITSPERPIXEL(SDL_PIXEL_FORMAT), SDL_PIXEL_FORMAT);
}

SDL_Surface* init_surface(const GLchar* const path) {
	SDL_Surface* const surface = SDL_LoadBMP(path);
	if (surface == NULL) fail("open texture file", OpenImageFile);

	if (surface -> format -> format == SDL_PIXEL_FORMAT) // Format is already correct
		return surface;
	else {
		SDL_Surface* const converted_surface = SDL_ConvertSurfaceFormat(surface, SDL_PIXEL_FORMAT, 0);
		deinit_surface(surface);
		return converted_surface;
	}
}

void use_texture(const GLuint texture, const GLuint shader_program,
	const GLchar* const sampler_name, const TextureType type, const byte texture_unit) {

	glActiveTexture(GL_TEXTURE0 + texture_unit);
	glBindTexture(type, texture);
	INIT_UNIFORM_VALUE_FROM_VARIABLE_NAME(sampler_name, shader_program, 1i, texture_unit);
}

GLuint preinit_texture(const TextureType type, const TextureWrapMode wrap_mode) {
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(type, texture);

	glTexParameteri(type, GL_TEXTURE_MAG_FILTER, OPENGL_TEX_MAG_FILTER);
	glTexParameteri(type, GL_TEXTURE_MIN_FILTER,
		(type == TexSkybox) ? OPENGL_SKYBOX_TEX_MIN_FILTER : OPENGL_TEX_MIN_FILTER);

	glTexParameteri(type, GL_TEXTURE_WRAP_S, wrap_mode);
	glTexParameteri(type, GL_TEXTURE_WRAP_T, wrap_mode);

	if (type == TexSkybox) glTexParameteri(type, GL_TEXTURE_WRAP_R, wrap_mode);
	else {
		#ifdef ENABLE_ANISOTROPIC_FILTERING
		float aniso;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
		glTexParameterf(type, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
		#endif
	}
	
	return texture;
}

// This function assumes that the surface is locked beforehand. This is legacy code and should be removed if possible
void write_surface_to_texture(SDL_Surface* const surface,
	const TextureType type, const GLenum internal_format) {

	const byte must_lock = SDL_MUSTLOCK(surface);
	if (must_lock) SDL_LockSurface(surface);

	glTexImage2D(type, 0, internal_format,
		surface -> w, surface -> h, 0, OPENGL_INPUT_PIXEL_FORMAT,
		GL_UNSIGNED_BYTE, surface -> pixels);

	if (must_lock) SDL_UnlockSurface(surface);
}

GLuint init_plain_texture(const GLchar* const path, const TextureType type,
	const TextureWrapMode wrap_mode, const GLenum internal_format) {

	const GLuint texture = preinit_texture(type, wrap_mode);
	SDL_Surface* const surface = init_surface(path);

	write_surface_to_texture(surface, TexPlain, internal_format);
	glGenerateMipmap(TexPlain);
	deinit_surface(surface);

	return texture;
}

static void init_still_subtextures_in_texture_set(
	const GLsizei num_still_subtextures, SDL_Surface* const rescaled_surface, va_list args) {

	for (GLsizei i = 0; i < num_still_subtextures; i++) {
		SDL_Surface *const surface = init_surface(va_arg(args, GLchar*)), *surface_copied_to_gpu;

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
		SDL_Surface* const spritesheet_surface = init_surface(va_arg(args, GLchar*));
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

	if (num_still_subtextures > MAX_NUM_SECTOR_SUBTEXTURES)
		fail("load textures; too many still subtextures", TextureIDIsTooLarge);

	va_list args, args_copy;
	va_start(args, rescale_h);
	va_copy(args_copy, args);

	////////// Getting number of animated frames for all animations

	GLsizei num_animated_frames = 0; // A frame is a subtexture

	for (GLsizei i = 0; i < num_still_subtextures; i++, va_arg(args_copy, GLchar*)); // Discarding still subtexture args
	for (GLsizei i = 0; i < num_animation_sets; i++) {
		va_arg(args_copy, GLchar*); // Discarding path, frames across, and frames down args
		va_arg(args_copy, GLsizei);
		va_arg(args_copy, GLsizei);
		num_animated_frames += va_arg(args_copy, GLsizei); // Adding num frames for one animation set
	}

	va_end(args_copy);

	////////// Defining texture and rescaled surface

	const GLsizei total_num_subtextures = num_still_subtextures + num_animated_frames;
	const GLuint texture = preinit_texture(TexSet, wrap_mode);

	glTexImage3D(TexSet, 0, OPENGL_DEFAULT_INTERNAL_PIXEL_FORMAT, rescale_w,
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
