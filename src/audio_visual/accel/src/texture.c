#ifndef TEXTURE_C
#define TEXTURE_C

#include "headers/texture.h"
#include "constants.c"

////////// Surface initialization

SDL_Surface* init_blank_surface(const GLsizei width, const GLsizei height, const SDL_PixelFormatEnum pixel_format_name) {
	return SDL_CreateRGBSurfaceWithFormat(0, width, height, SDL_BITSPERPIXEL(pixel_format_name), pixel_format_name);
}

SDL_Surface* init_surface(const GLchar* const path) {
	SDL_Surface* const surface = SDL_LoadBMP(path);
	if (surface == NULL) fail("open texture file", OpenImageFile);

	if (surface -> format -> format == SDL_PIXEL_FORMAT)
		return surface; // Format is already correct
	else {
		SDL_Surface* const converted_surface = SDL_ConvertSurfaceFormat(surface, SDL_PIXEL_FORMAT, 0);
		deinit_surface(surface);
		return converted_surface;
	}
}

////////// Texture state setting utilities

void set_sampler_texture_unit_for_shader(const GLchar* const sampler_name, const GLuint shader, const byte texture_unit) {
	INIT_UNIFORM_VALUE_FROM_VARIABLE_NAME(sampler_name, shader, 1i, texture_unit); // Sets texture unit for shader
}

void set_current_texture_unit(const byte texture_unit) {
	glActiveTexture(GL_TEXTURE0 + texture_unit);
}

void use_texture(const GLuint texture, const GLuint shader,
	const GLchar* const sampler_name, const TextureType type, const byte texture_unit) {

	set_sampler_texture_unit_for_shader(sampler_name, shader, texture_unit);
	set_current_texture_unit(texture_unit);
	set_current_texture(type, texture); // Associates the input texture with the right texture unit
}

//////////

// This sets the current texture to be the returned texture
GLuint preinit_texture(const TextureType type, const TextureWrapMode wrap_mode,
	const TextureFilterMode mag_filter, const TextureFilterMode min_filter) {

	GLuint texture;
	glGenTextures(1, &texture);
	set_current_texture(type, texture);

	glTexParameteri(type, GL_TEXTURE_MAG_FILTER, (GLint) mag_filter);
	glTexParameteri(type, GL_TEXTURE_MIN_FILTER, (GLint) min_filter);

	const GLint cast_wrap_mode = (GLint) wrap_mode;
	glTexParameteri(type, GL_TEXTURE_WRAP_S, cast_wrap_mode);
	glTexParameteri(type, GL_TEXTURE_WRAP_T, cast_wrap_mode);

	if (type == TexSkybox) glTexParameteri(type, GL_TEXTURE_WRAP_R, cast_wrap_mode);

	#ifdef ENABLE_ANISOTROPIC_FILTERING

	/* Checking if the extension is available at runtime. Also, skyboxes get no anisotropic
	filtering, because they are usually magnified and are not viewed at very steep angles. */
	else if (GLAD_GL_EXT_texture_filter_anisotropic) {
		const GLfloat aniso_filtering_level = get_runtime_constant(AnisotropicFilteringLevel);
		glTexParameterf(type, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso_filtering_level);
	}

	#endif

	return texture;
}

void write_surface_to_texture(SDL_Surface* const surface,
	const TextureType type, const GLint internal_format) {

	WITH_SURFACE_PIXEL_ACCESS(surface,
		glTexImage2D(type, 0, internal_format, surface -> w,
			surface -> h, 0, OPENGL_INPUT_PIXEL_FORMAT,
			OPENGL_COLOR_CHANNEL_TYPE, surface -> pixels);
	);
}

GLuint init_plain_texture(const GLchar* const path, const TextureType type,
	const TextureWrapMode wrap_mode, const TextureFilterMode mag_filter,
	const TextureFilterMode min_filter, const GLint internal_format) {

	const GLuint texture = preinit_texture(type, wrap_mode, mag_filter, min_filter);
	SDL_Surface* const surface = init_surface(path);

	write_surface_to_texture(surface, TexPlain, internal_format);
	glGenerateMipmap(TexPlain);
	deinit_surface(surface);

	return texture;
}

static void init_still_subtextures_in_texture_set(const GLsizei num_still_subtextures,
	const GLchar* const* const still_subtexture_paths, SDL_Surface* const rescaled_surface) {

	const GLsizei correct_w = rescaled_surface -> w, correct_h = rescaled_surface -> h;

	for (GLsizei i = 0; i < num_still_subtextures; i++) {
		SDL_Surface *const surface = init_surface(still_subtexture_paths[i]), *surface_with_right_size;

		if (surface -> w != correct_w || surface -> h != correct_h) {
			SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
			SDL_BlitScaled(surface, NULL, rescaled_surface, NULL);
			surface_with_right_size = rescaled_surface;
		}
		else surface_with_right_size = surface;

		WITH_SURFACE_PIXEL_ACCESS(surface_with_right_size,
			glTexSubImage3D(TexSet, 0, 0, 0, i,
				correct_w, correct_h, 1, OPENGL_INPUT_PIXEL_FORMAT,
				OPENGL_COLOR_CHANNEL_TYPE, surface_with_right_size -> pixels);
		);

		deinit_surface(surface);
	}
}

static void init_animated_subtextures_in_texture_set(const GLsizei num_animated_frames,
	const GLsizei num_still_subtextures, SDL_Surface* const rescaled_surface, va_list args) {

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

			WITH_SURFACE_PIXEL_ACCESS(rescaled_surface,
				glTexSubImage3D(TexSet, 0, 0, 0, animation_frame_index,
					rescaled_surface -> w, rescaled_surface -> h, 1, OPENGL_INPUT_PIXEL_FORMAT,
					OPENGL_COLOR_CHANNEL_TYPE, rescaled_surface -> pixels);
			);
		}
		deinit_surface(spritesheet_surface);
	}
}

GLuint init_texture_set(const TextureWrapMode wrap_mode, const TextureFilterMode mag_filter,
	const TextureFilterMode min_filter, const GLsizei num_still_subtextures,
	const GLsizei num_animation_sets, const GLsizei rescale_w, const GLsizei rescale_h,
	const GLchar* const* const still_subtexture_paths, ...) {

	if (num_still_subtextures > MAX_NUM_SECTOR_SUBTEXTURES)
		fail("load textures; too many still subtextures", TextureIDIsTooLarge);

	va_list args, args_copy;
	va_start(args, still_subtexture_paths);
	va_copy(args_copy, args);

	////////// Getting number of animated frames for all animations

	GLsizei num_animated_frames = 0; // A frame is a subtexture

	for (GLsizei i = 0; i < num_animation_sets; i++) {
		va_arg(args_copy, GLchar*); // Discarding path, frames across, and frames down args
		va_arg(args_copy, GLsizei);
		va_arg(args_copy, GLsizei);
		num_animated_frames += va_arg(args_copy, GLsizei); // Adding num frames for one animation set
	}

	va_end(args_copy);

	////////// Defining texture, and a rescaled surface

	const GLsizei total_num_subtextures = num_still_subtextures + num_animated_frames;
	const GLuint texture = preinit_texture(TexSet, wrap_mode, mag_filter, min_filter);

	glTexImage3D(TexSet, 0, OPENGL_DEFAULT_INTERNAL_PIXEL_FORMAT, rescale_w,
		rescale_h, total_num_subtextures, 0, OPENGL_INPUT_PIXEL_FORMAT,
		OPENGL_COLOR_CHANNEL_TYPE, NULL);

	SDL_Surface* const rescaled_surface = init_blank_surface(rescale_w, rescale_h, SDL_PIXEL_FORMAT);

	////////// Filling array texture with still and animated subtextures

	init_still_subtextures_in_texture_set(num_still_subtextures, still_subtexture_paths, rescaled_surface);
	init_animated_subtextures_in_texture_set(num_animated_frames, num_still_subtextures, rescaled_surface, args);
	glGenerateMipmap(TexSet);

	////////// Deinitialization

	deinit_surface(rescaled_surface);
	va_end(args);

	return texture;
}

#endif
