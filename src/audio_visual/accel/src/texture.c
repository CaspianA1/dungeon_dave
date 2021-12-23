#ifndef TEXTURE_C
#define TEXTURE_C

#include "headers/texture.h"
#include "headers/utils.h"

static SDL_Surface* init_blank_surface(const int width, const int height) {
	return SDL_CreateRGBSurfaceWithFormat(0, width, height, SDL_BITSPERPIXEL(SDL_PIXEL_FORMAT), SDL_PIXEL_FORMAT);
}

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

		write_surface_to_texture(surface, TexPlain);
		glGenerateMipmap(TexPlain);
		deinit_surface(surface);
	}

	va_end(args);
	return textures;
}

static GLsizei get_num_subtextures_for_multi_texture(va_list args, const GLsizei num_multi_textures) {
	va_list args_copy;
	va_copy(args_copy, args);

	GLsizei num_subtextures_in_set = 0;

	for (GLsizei i = 0; i < num_multi_textures; i++) {
		va_arg(args_copy, char*); // Discarding path

		if (va_arg(args_copy, unsigned)) { // If animated
			va_arg(args_copy, GLsizei); // Discarding frames across and down
			va_arg(args_copy, GLsizei);
			num_subtextures_in_set += va_arg(args_copy, GLsizei); // Adding total_frames
		}
		else num_subtextures_in_set++;
	}

	if (num_subtextures_in_set > GL_MAX_ARRAY_TEXTURE_LAYERS)
		fail("put textures in a texture set because it exceeds the max array texture layers",
			TextureSetIsTooLarge);

	va_end(args_copy);

	return num_subtextures_in_set;
}

// Path, is animated. If animated: frames across, frames down, total_frames
GLuint init_multi_textures(const GLsizei num_multi_textures,
	const GLsizei rescale_w, const GLsizei rescale_h, ...) {

	va_list args;
	va_start(args, rescale_h);

	const GLsizei num_subtextures_in_set = get_num_subtextures_for_multi_texture(args, num_multi_textures);
	const GLuint texture = preinit_texture(TexSet, TexNonRepeating);

	glTexImage3D(TexSet, 0, OPENGL_INTERNAL_PIXEL_FORMAT,
		rescale_w, rescale_h, num_subtextures_in_set, 0,
		OPENGL_INPUT_PIXEL_FORMAT, OPENGL_COLOR_CHANNEL_TYPE, NULL);

	SDL_Surface* const rescaling_surface = init_blank_surface(rescale_w, rescale_h);

	for (GLsizei i = 0, frame_index = 0; i < num_multi_textures; i++) {
		const char* const path = va_arg(args, char*);
		DEBUG(path, s);

		SDL_Surface* const surface = init_surface(path);

		const byte is_animated = va_arg(args, unsigned);

		if (is_animated) { // TODO: animation part of init_multi_textures
			const GLsizei
				frames_across = va_arg(args, GLsizei),
				frames_down = va_arg(args, GLsizei),
				total_frames = va_arg(args, GLsizei);

			DEBUG(frames_across, d);
			DEBUG(frames_down, d);
			DEBUG(total_frames, d);
		}
		else {
			puts("A plain texture");
			const SDL_Surface* cpu_src;

			if (surface -> w != rescale_w || surface -> h != rescale_h) {
				SDL_UnlockSurface(rescaling_surface);
				SDL_UnlockSurface(surface);
				SDL_SoftStretchLinear(surface, NULL, rescaling_surface, NULL);
				SDL_LockSurface(surface);
				SDL_LockSurface(rescaling_surface);
				cpu_src = rescaling_surface;
			}
			else cpu_src = surface;

			glTexSubImage3D(TexSet, 0, 0, 0, frame_index,
				rescale_w, rescale_h, 1, OPENGL_INPUT_PIXEL_FORMAT,
				OPENGL_COLOR_CHANNEL_TYPE, cpu_src -> pixels);

			frame_index++;
		}

		puts("---");

		deinit_surface(surface);
	}

	//////////

	deinit_surface(rescaling_surface);
	va_end(args);
	return texture;
}

// TODO: hybrid init_texture_set and init_animation
GLuint init_animation(const char* const path, const GLsizei frames_across,
	const GLsizei frames_down, const GLsizei total_frames) {

	SDL_Surface* const spritesheet_surface = init_surface(path);
	SDL_UnlockSurface(spritesheet_surface);
	// If blending enabled, consecutive blits to `frame_surface` will mix with previous blits
	SDL_SetSurfaceBlendMode(spritesheet_surface, SDL_BLENDMODE_NONE);

	SDL_Rect spritesheet_copy_area = {.w = spritesheet_surface -> w / frames_across, .h = spritesheet_surface -> h / frames_down};
	SDL_Rect frame_copy_area = {.x = 0, .y = 0, .w = spritesheet_copy_area.w, .h = spritesheet_copy_area.h};

	/* Needed b/c glTexSubImage3D can't copy pixels from the main surface; otherwise, layout
	of pixels in `spritesheet_surface` will result in other frames being partially copied */
	SDL_Surface* const frame_surface = init_blank_surface(spritesheet_copy_area.w, spritesheet_copy_area.h);
	const Uint32* const frame_pixels = frame_surface -> pixels;

	const GLuint texture = preinit_texture(TexSet, TexNonRepeating);

	glTexImage3D(TexSet, 0, OPENGL_INTERNAL_PIXEL_FORMAT,
		spritesheet_copy_area.w, spritesheet_copy_area.h, total_frames,
		0, OPENGL_INPUT_PIXEL_FORMAT, OPENGL_COLOR_CHANNEL_TYPE, NULL);

	for (GLsizei frame_index = 0; frame_index < total_frames; frame_index++) {
		// Frame index x = frame_index % frames_across, and y = frame_index / frames_across
		spritesheet_copy_area.x = (frame_index % frames_across) * spritesheet_copy_area.w;
		spritesheet_copy_area.y = (frame_index / frames_across) * spritesheet_copy_area.h;

		SDL_UnlockSurface(spritesheet_surface);
		SDL_LowerBlit(spritesheet_surface, &spritesheet_copy_area, frame_surface, &frame_copy_area);
		SDL_LockSurface(spritesheet_surface);

		glTexSubImage3D(TexSet, 0, 0, 0, frame_index,
			spritesheet_copy_area.w, spritesheet_copy_area.h,
			1, OPENGL_INPUT_PIXEL_FORMAT, OPENGL_COLOR_CHANNEL_TYPE, frame_pixels);
	}

	deinit_surface(frame_surface);
	deinit_surface(spritesheet_surface);
	glGenerateMipmap(TexSet);
	return texture;
}

// Param: Texture path
GLuint init_texture_set(const TextureWrapMode wrap_mode,
	const GLsizei subtex_width, const GLsizei subtex_height, const GLsizei num_textures, ...) {

	const GLuint ts = preinit_texture(TexSet, wrap_mode);

	glTexImage3D(TexSet, 0, OPENGL_INTERNAL_PIXEL_FORMAT,
		subtex_width, subtex_height, num_textures,
		0, OPENGL_INPUT_PIXEL_FORMAT, OPENGL_COLOR_CHANNEL_TYPE, NULL);
	
	SDL_Surface* const rescaled_surface = init_blank_surface(subtex_width, subtex_height);

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

		glTexSubImage3D(TexSet, 0, 0, 0, i, subtex_width, subtex_height, 1,
			OPENGL_INPUT_PIXEL_FORMAT, OPENGL_COLOR_CHANNEL_TYPE, src_surface -> pixels);

		deinit_surface(surface);
	}

	deinit_surface(rescaled_surface);
	glGenerateMipmap(TexSet);
	va_end(args);

	return ts;
}

#endif
