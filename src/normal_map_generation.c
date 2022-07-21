#ifndef NORMAL_MAP_GENERATION_C
#define NORMAL_MAP_GENERATION_C

#include "headers/normal_map_generation.h"
#include "headers/texture.h"
#include "headers/constants.h"

static GLint int_min(const GLint val, const GLint lower) {
	return (val < lower) ? val : lower;
}

static GLint int_max(const GLint val, const GLint upper) {
	return (val > upper) ? val : upper;
}

static GLint limit_int_to_domain(const GLint val, const GLint lower, const GLint upper) {
	return int_min(int_max(val, lower), upper);
}

static GLfloat sobel_sample(const SDL_Surface* const surface, const GLint x, const GLint y) {
	const sdl_pixel_t pixel = *(sdl_pixel_t*) read_surface_pixel(surface, x, y);

	sdl_pixel_component_t r, g, b;
	SDL_GetRGB(pixel, surface -> format, &r, &g, &b);

	static const GLfloat one_third = 1.0f / 3.0f;
	return (r + g + b) * one_third;
}

/* This function is based on these sources:
- https://en.wikipedia.org/wiki/Sobel_operator
- https://www.shadertoy.com/view/Xtd3DS

The strength of a pixel's color is considered
to be the average of its three color components.

It's assumed that `src` has the same size as `dest`. */
static void generate_normal_map(SDL_Surface* const src, SDL_Surface* const dest, const GLint subtexture_h, const GLfloat intensity) {
	const GLint w = src -> w, h = src -> h;
	const SDL_PixelFormat* const format = src -> format;

	const GLfloat
		one_over_intensity_on_rgb_scale = constants.max_byte_value / intensity,
		half_max_byte_value = 0.5f * constants.max_byte_value;

	WITH_SURFACE_PIXEL_ACCESS(src,
		WITH_SURFACE_PIXEL_ACCESS(dest,

			for (GLint y = 0; y < h; y++) {
				sdl_pixel_t* const dest_pixel = read_surface_pixel(dest, 0, y);

				const GLint subtexture_top = (y / subtexture_h) * subtexture_h;
				const GLint subtexture_bottom = subtexture_top + subtexture_h - 1;

				for (GLint x = 0; x < w; x++) {
					const GLint
						left_x = int_max(x - 1, 0),             right_x = int_min(x + 1, w - 1),
						top_y = int_max(y - 1, subtexture_top), bottom_y = int_min(y + 1, subtexture_bottom);

					const GLfloat // These samples are in a range from 0 to `constants.max_byte_value`
						tl = sobel_sample(src, left_x, top_y),  tm = sobel_sample(src, x, top_y),
						tr = sobel_sample(src, right_x, top_y), ml = sobel_sample(src, left_x, y),
						mr = sobel_sample(src, right_x, y),     bl = sobel_sample(src, left_x, bottom_y),
						bm = sobel_sample(src, x, bottom_y),    br = sobel_sample(src, right_x, bottom_y);

					vec3 normal = { // The x and y components of this are the result of the Sobel operator
						(-tl - ml * 2.0f - bl) + (tr + mr * 2.0f + br),
						(-tl - tm * 2.0f - tr) + (bl + bm * 2.0f + br),
						one_over_intensity_on_rgb_scale
					};

					/* This first normalizes the normal, and then converts
					the normal from a range of (-1, 1) to (0, `max_byte_value`) */
					glm_vec3_normalize(normal);
					glm_vec3_scale(normal, half_max_byte_value, normal);
					glm_vec3_adds(normal, half_max_byte_value, normal);

					dest_pixel[x] = SDL_MapRGB(format,
						(sdl_pixel_component_t) normal[0],
						(sdl_pixel_component_t) normal[1],
						(sdl_pixel_component_t) normal[2]
					);
				}
			}
		);
	);
}

////////// This code concerns Gaussian blur (the normal map input is blurred to cut out high frequencies from the Sobel operator).

static GLfloat* compute_1D_gaussian_kernel(const signed_byte radius, const GLfloat std_dev) {
	const signed_byte kernel_length = radius * 2 + 1;

	GLfloat* const kernel = malloc((size_t) kernel_length * sizeof(GLfloat)), sum = 0.0f;
	const GLfloat one_over_two_times_std_dev_squared = 1.0f / (2.0f * std_dev * std_dev);

	for (signed_byte x = 0; x < kernel_length; x++) {
		const signed_byte dx = x - radius;
		const GLfloat weight = expf(-(dx * dx) * one_over_two_times_std_dev_squared);
		kernel[x] = weight;
		sum += weight;
	}

	const GLfloat one_over_sum = 1.0f / sum;
	for (signed_byte i = 0; i < kernel_length; i++) kernel[i] *= one_over_sum;

	return kernel;
}

// It's assumed that `src` has the same size as `dest`, and that they both have the same format.
static void do_separable_gaussian_blur_pass(
	SDL_Surface* const src, SDL_Surface* const dest,
	const GLfloat* const kernel, const GLint subtexture_h,
	const signed_byte kernel_radius, const bool blur_is_vertical) {

	const GLint w = src -> w, h = src -> h;

	const SDL_PixelFormat *const format = src -> format;

	WITH_SURFACE_PIXEL_ACCESS(src,
		WITH_SURFACE_PIXEL_ACCESS(dest,

			for (GLint y = 0; y < h; y++) {
				sdl_pixel_t* const dest_pixel = read_surface_pixel(dest, 0, y);

				const GLint subtexture_top = (y / subtexture_h) * subtexture_h;
				const GLint subtexture_bottom = subtexture_top + subtexture_h - 1;

				for (GLint x = 0; x < w; x++) {
					vec3 summed_channels = GLM_VEC3_ZERO_INIT;

					for (signed_byte i = -kernel_radius; i <= kernel_radius; i++) {
						GLint fx = x, fy = y; // `f` = filter
						if (blur_is_vertical) fy += i; else fx += i;

						const sdl_pixel_t src_pixel = *(sdl_pixel_t*) read_surface_pixel(src,
							limit_int_to_domain(fx, 0, w - 1),
							limit_int_to_domain(fy, subtexture_top, subtexture_bottom)
						);

						sdl_pixel_component_t r, g, b;
						SDL_GetRGB(src_pixel, format, &r, &g, &b);

						glm_vec3_muladds((vec3) {r, g, b}, kernel[i + kernel_radius], summed_channels);
					}

					dest_pixel[x] = SDL_MapRGB(format,
						(sdl_pixel_component_t) summed_channels[0],
						(sdl_pixel_component_t) summed_channels[1],
						(sdl_pixel_component_t) summed_channels[2]
					);
				}
			}
		);
	);
}

GLuint init_normal_map_from_diffuse_texture_set(const GLuint diffuse_texture_set, const NormalMapConfig* const config) {
	/* How this function works:

	- First, query OpenGL about information about the texture set, like its dimensions, and its filters used.
	- Then, define two general purpose surfaces, #1 and #2.
	- Copy the texture contents into surface #1.
	- Blur #1 horizontally to #2, and blur #2 vertically to #1.
	- Generate a normal map of #1 to #2.
	- After that, reupload #2 to the GPU as a texture set of normal maps.

	Note: normal maps are not interleaved with the texture set because if gamma correction is used,
	the texture set will be in SRGB, and normal maps should be in a linear color space. */

	////////// Querying OpenGL for information about the texture set

	glBindTexture(TexSet, diffuse_texture_set);

	GLint subtexture_w, subtexture_h, num_subtextures, wrap_mode, mag_filter, min_filter;
	glGetTexLevelParameteriv(TexSet, 0, GL_TEXTURE_WIDTH, &subtexture_w);
	glGetTexLevelParameteriv(TexSet, 0, GL_TEXTURE_HEIGHT, &subtexture_h);
	glGetTexLevelParameteriv(TexSet, 0, GL_TEXTURE_DEPTH, &num_subtextures);

	// Wrap mode for each axis is the same, so only for 'S' (or across) is fine
	glGetTexParameteriv(TexSet, GL_TEXTURE_WRAP_S, &wrap_mode);
	glGetTexParameteriv(TexSet, GL_TEXTURE_MAG_FILTER, &mag_filter);
	glGetTexParameteriv(TexSet, GL_TEXTURE_MIN_FILTER, &min_filter);

	////////// Uploading the texture to the CPU

	const GLint general_purpose_surfaces_h = subtexture_h * num_subtextures;

	SDL_Surface
		*const general_purpose_surface_1 = init_blank_surface(subtexture_w, general_purpose_surfaces_h, SDL_PIXEL_FORMAT),
		*const general_purpose_surface_2 = init_blank_surface(subtexture_w, general_purpose_surfaces_h, SDL_PIXEL_FORMAT);

	// Copying the original texture set to #1
	WITH_SURFACE_PIXEL_ACCESS(general_purpose_surface_1,
		glGetTexImage(TexSet, 0, OPENGL_INPUT_PIXEL_FORMAT,
			OPENGL_COLOR_CHANNEL_TYPE, general_purpose_surface_1 -> pixels);
	);

	////////// Blurring it (if needed), and then making a normal map

	const signed_byte blur_radius = config -> blur_radius;

	if (blur_radius != 0) {
		GLfloat* const blur_kernel = compute_1D_gaussian_kernel(blur_radius, config -> blur_std_dev);

		do_separable_gaussian_blur_pass( // Blurring #1 to #2 horizontally
			general_purpose_surface_1, general_purpose_surface_2,
			blur_kernel, subtexture_h, blur_radius, false);

		do_separable_gaussian_blur_pass( // Blurring #2 to #1 vertically
			general_purpose_surface_2, general_purpose_surface_1,
			blur_kernel, subtexture_h, blur_radius, true);

		free(blur_kernel);
	}

	// Making a normal map of #1 to #2
	generate_normal_map(general_purpose_surface_1, general_purpose_surface_2, subtexture_h, config -> intensity);

	////////// Making a new texture on the GPU, and then writing the normal map to that

	const GLuint normal_map_set = preinit_texture(TexSet,
		(TextureWrapMode) wrap_mode,
		(TextureFilterMode) mag_filter,
		(TextureFilterMode) min_filter, false);

	// Copying #2 to a new texture on the GPU
	WITH_SURFACE_PIXEL_ACCESS(general_purpose_surface_2,
		glTexImage3D(TexSet, 0, OPENGL_NORMAL_MAP_INTERNAL_PIXEL_FORMAT, subtexture_w,
			subtexture_h, num_subtextures, 0, OPENGL_INPUT_PIXEL_FORMAT,
			OPENGL_COLOR_CHANNEL_TYPE, general_purpose_surface_2 -> pixels);
	);

	glGenerateMipmap(TexSet);

	////////// Deinitialization

	deinit_surface(general_purpose_surface_1);
	deinit_surface(general_purpose_surface_2);

	return normal_map_set;
}

#endif
