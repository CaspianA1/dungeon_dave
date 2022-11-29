#include "utils/texture.h"
#include "utils/failure.h" // For `FAIL`
#include <limits.h> // For `CHAR_BIT`
#include "utils/macro_utils.h" // For `ON_FIRST_CALL`
#include "data/constants.h" // For `engine.enabled.anisotropic_filtering`, and `max_byte_value`
#include "utils/opengl_wrappers.h" // For various OpenGL wrappers

//////////

// This is written to by `window_creation.h`.
GLfloat global_anisotropic_filtering_level;

////////// Surface initialization and alpha premultiplication

SDL_Surface* init_blank_surface(const GLsizei width, const GLsizei height) {
	SDL_Surface* const blank_surface = SDL_CreateRGBSurfaceWithFormat(
		0, width, height, SDL_BITSPERPIXEL(SDL_PIXEL_FORMAT), SDL_PIXEL_FORMAT);

	if (blank_surface == NULL) FAIL(CreateSurface, "Could not create a blank surface: %s", SDL_GetError());
	return blank_surface;
}

SDL_Surface* init_blank_grayscale_surface(const GLsizei width, const GLsizei height) {
	SDL_Surface* const blank_surface = SDL_CreateRGBSurface(0, width, height, CHAR_BIT, 0, 0, 0, 0);
	if (blank_surface == NULL) FAIL(CreateSurface, "Could not create a blank grayscale surface: %s", SDL_GetError());

	static const uint16_t num_palette_colors = SDL_MAX_UINT8 + 1u;
	static SDL_Color palette_colors[num_palette_colors];

	ON_FIRST_CALL(
		for (uint16_t i = 0; i < num_palette_colors; i++) {
			SDL_Color* const color = palette_colors + i;
			color -> r = color -> g = color -> b = (Uint8) i;
			color -> a = constants.max_byte_value;
		}
	);

	SDL_SetPaletteColors(blank_surface -> format -> palette, palette_colors, 0, num_palette_colors);
	return blank_surface;
}

SDL_Surface* init_surface(const GLchar* const path) {
	SDL_Surface* const surface = SDL_LoadBMP(path);
	if (surface == NULL) FAIL(OpenFile, "Could not load a surface from disk: %s", SDL_GetError());

	if (surface -> format -> format == SDL_PIXEL_FORMAT)
		return surface; // Format is already correct
	else {
		SDL_Surface* const converted_surface = SDL_ConvertSurfaceFormat(surface, SDL_PIXEL_FORMAT, 0);
		deinit_surface(surface);
		return converted_surface;
	}
}

void* read_surface_pixel(const SDL_Surface* const surface, const GLint x, const GLint y) {
	sdl_pixel_component_t* const row = (sdl_pixel_component_t*) surface -> pixels + y * surface -> pitch;
	return row + x * surface -> format -> BytesPerPixel;
}

static void premultiply_surface_alpha(SDL_Surface* const surface) {
	const GLint w = surface -> w, h = surface -> h;
	const SDL_PixelFormat* const format = surface -> format;

	const GLfloat one_over_max_byte_value = 1.0f / constants.max_byte_value;
	sdl_pixel_component_t r, g, b, a;

	WITH_SURFACE_PIXEL_ACCESS(surface,
		for (GLint y = 0; y < h; y++) {
			sdl_pixel_t* const row = read_surface_pixel(surface, 0, y);

			for (GLint x = 0; x < w; x++) {
				sdl_pixel_t* const pixel = row + x;
				SDL_GetRGBA(*pixel, format, &r, &g, &b, &a);

				const GLfloat normalized_alpha = a * one_over_max_byte_value;

				r = (sdl_pixel_component_t) (r * normalized_alpha);
				g = (sdl_pixel_component_t) (g * normalized_alpha);
				b = (sdl_pixel_component_t) (b * normalized_alpha);

				*pixel = SDL_MapRGBA(format, r, g, b, a);
			}
		}
	);
}

////////// Texture state setting utilities

void use_texture_in_shader(const GLuint texture,
	const GLuint shader, const GLchar* const sampler_name,
	const TextureType type, const TextureUnit texture_unit) {

	glUniform1i(safely_get_uniform(shader, sampler_name), (GLint) texture_unit); // Sets the texture unit for the inputted shader
	glActiveTexture(GL_TEXTURE0 + texture_unit); // Sets the current active texture unit
	use_texture(type, texture); // Associates the input texture with the right texture unit
}

//////////

// This sets the current texture to be the returned texture. TODO: allow different wrap modes for S, T, and R.
GLuint preinit_texture(const TextureType type, const TextureWrapMode wrap_mode,
	const TextureFilterMode mag_filter, const TextureFilterMode min_filter,
	const bool use_anisotropic_filtering) {

	GLuint texture;
	glGenTextures(1, &texture);
	use_texture(type, texture);

	glTexParameteri(type, GL_TEXTURE_MAG_FILTER, (GLint) mag_filter);
	glTexParameteri(type, GL_TEXTURE_MIN_FILTER, (GLint) min_filter);

	const GLint cast_wrap_mode = (GLint) wrap_mode;
	glTexParameteri(type, GL_TEXTURE_WRAP_S, cast_wrap_mode);

	if (type != TexPlain1D) glTexParameteri(type, GL_TEXTURE_WRAP_T, cast_wrap_mode);
	if (type == TexSkybox || type == TexVolumetric) glTexParameteri(type, GL_TEXTURE_WRAP_R, cast_wrap_mode);

	if (use_anisotropic_filtering && GLAD_GL_EXT_texture_filter_anisotropic)
		glTexParameterf(type, GL_TEXTURE_MAX_ANISOTROPY_EXT, global_anisotropic_filtering_level);

	return texture;
}

void init_texture_data(const TextureType type, const GLsizei* const size,
	const GLenum input_format, const GLint internal_format, const GLenum color_channel_type,
	const void* const pixels) {

	const GLint level = 0, border = 0;

	#define UPLOAD_CALL(target, dims, ...) glTexImage##dims##D(\
		target, level, internal_format, __VA_ARGS__, border, input_format, color_channel_type, pixels\
	); break

	switch (type) {
		case TexPlain1D: UPLOAD_CALL(type, 1, size[0]);
		case TexPlain: UPLOAD_CALL(type, 2, size[0], size[1]);

		case TexSkybox: {
			const GLsizei face_size = size[0];
			UPLOAD_CALL(GL_TEXTURE_CUBE_MAP_POSITIVE_X + (GLenum) size[1], 2, face_size, face_size);
		}

		case TexSet: case TexVolumetric: UPLOAD_CALL(type, 3, size[0], size[1], size[2]);
	}

	#undef UPLOAD_CALL
}

static void init_still_subtextures_in_texture_set(
	const bool premultiply_alpha, const texture_id_t num_still_subtextures,
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

		if (premultiply_alpha) premultiply_surface_alpha(surface_with_right_size);

		WITH_SURFACE_PIXEL_ACCESS(surface_with_right_size,
			glTexSubImage3D(TexSet, 0, 0, 0, i,
				correct_w, correct_h, 1, OPENGL_INPUT_PIXEL_FORMAT,
				OPENGL_COLOR_CHANNEL_TYPE, surface_with_right_size -> pixels);
		);

		deinit_surface(surface);
	}
}

static void init_animated_subtextures_in_texture_set(const bool premultiply_alpha,
	const texture_id_t num_animated_frames, const texture_id_t num_still_subtextures,
	const AnimationLayout* const animation_layouts, SDL_Surface* const rescaled_surface) {

	for (texture_id_t animation_layout_index = 0, animation_frame_index = num_still_subtextures;
		animation_frame_index < num_animated_frames; animation_layout_index++) {

		const AnimationLayout animation_layout = animation_layouts[animation_layout_index];

		SDL_Surface* const spritesheet_surface = init_surface(animation_layout.spritesheet_path);
		SDL_SetSurfaceBlendMode(spritesheet_surface, SDL_BLENDMODE_NONE);

		if (premultiply_alpha) premultiply_surface_alpha(spritesheet_surface);

		SDL_Rect spritesheet_frame_area = {
			.w = spritesheet_surface -> w / animation_layout.frames_across,
			.h = spritesheet_surface -> h / animation_layout.frames_down
		};

		for (texture_id_t frame_index = 0; frame_index < animation_layout.total_frames; frame_index++, animation_frame_index++) {
			const div_t frame_indices_across_and_down = div((int) frame_index, (int) animation_layout.frames_across);
			spritesheet_frame_area.x = frame_indices_across_and_down.rem * spritesheet_frame_area.w;
			spritesheet_frame_area.y = frame_indices_across_and_down.quot * spritesheet_frame_area.h;

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

GLuint init_texture_set(const bool premultiply_alpha,
	const TextureWrapMode wrap_mode, const TextureFilterMode mag_filter,
	const TextureFilterMode min_filter, const texture_id_t num_still_subtextures,
	const texture_id_t num_animation_layouts, const GLsizei rescale_w, const GLsizei rescale_h,
	const GLchar* const* const still_subtexture_paths, const AnimationLayout* const animation_layouts) {

	texture_id_t num_animated_frames = 0; // A frame is a subtexture
	for (texture_id_t i = 0; i < num_animation_layouts; i++) num_animated_frames += animation_layouts[i].total_frames;

	////////// Defining the texture set, and a rescaled surface

	const GLuint texture = preinit_texture(TexSet, wrap_mode, mag_filter, min_filter, true);

	init_texture_data(TexSet, (GLsizei[]) {rescale_w, rescale_h, num_still_subtextures + num_animated_frames},
		OPENGL_INPUT_PIXEL_FORMAT, OPENGL_DEFAULT_INTERNAL_PIXEL_FORMAT, OPENGL_COLOR_CHANNEL_TYPE, NULL);

	SDL_Surface* const rescaled_surface = init_blank_surface(rescale_w, rescale_h);

	////////// Filling the texture set with the still and animated subtextures, deiniting the rescaled surface, and returning

	init_still_subtextures_in_texture_set(premultiply_alpha, num_still_subtextures, still_subtexture_paths, rescaled_surface);
	init_animated_subtextures_in_texture_set(premultiply_alpha, num_animated_frames, num_still_subtextures, animation_layouts, rescaled_surface);
	init_texture_mipmap(TexSet);
	deinit_surface(rescaled_surface);

	return texture;
}

GLuint init_plain_texture(const GLchar* const path,
	const TextureWrapMode wrap_mode, const TextureFilterMode mag_filter,
	const TextureFilterMode min_filter, const GLint internal_format) {

	const TextureType type = TexPlain;

	const GLuint texture = preinit_texture(type, wrap_mode, mag_filter, min_filter, false);
	SDL_Surface* const surface = init_surface(path);

	WITH_SURFACE_PIXEL_ACCESS(surface,
		init_texture_data(type,
			(GLsizei[]) {surface -> w, surface -> h}, OPENGL_INPUT_PIXEL_FORMAT,
			internal_format, OPENGL_COLOR_CHANNEL_TYPE, surface -> pixels);
	);

	init_texture_mipmap(type);
	deinit_surface(surface);

	return texture;
}
