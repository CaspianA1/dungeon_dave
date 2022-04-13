#ifndef NORMAL_MAP_GENERATION_C
#define NORMAL_MAP_GENERATION_C

#include "headers/normal_map_generation.h"
#include "headers/constants.h"
#include "texture.c"

static int limit_int_to_domain(const int val, const int lower, const int upper) {
	if (val < lower) return lower;
	else if (val > upper) return upper;
	else return val;
}

static void* read_surface_pixel(const SDL_Surface* const surface, const int x, const int y) {
	sdl_pixel_component_t* const row = (sdl_pixel_component_t*) surface -> pixels + y * surface -> pitch;
	return row + x * (int) sizeof(sdl_pixel_t);
}

// If a coordinate (x or y) is out of bounds, it is converted to the closest possible edge value.
static void* edge_checked_read_surface_pixel(const SDL_Surface* const surface, int x, int y) {
	x = limit_int_to_domain(x, 0, surface -> w - 1);
	y = limit_int_to_domain(y, 0, surface -> h - 1);
	return read_surface_pixel(surface, x, y);
}

static float sobel_sample(const SDL_Surface* const surface, const int x, const int y) {
	const sdl_pixel_t pixel = *(sdl_pixel_t*) edge_checked_read_surface_pixel(surface, x, y);

	sdl_pixel_component_t r, g, b;
	SDL_GetRGB(pixel, surface -> format, &r, &g, &b);

	// This equation is from https://en.wikipedia.org/wiki/Relative_luminance
	const float luminance = r * 0.2126f + g * 0.7152f + b * 0.0722f; // This ranges from 0 to `max_byte_value`
	return luminance / constants.max_byte_value; // Normalized from 0 to 1
}

/* This function is based on these sources:
- https://en.wikipedia.org/wiki/Sobel_operator
- https://www.shadertoy.com/view/Xtd3DS

Also, this function computes luminance values
of pixels to use those as heightmap values.

It's assumed that `src` has the same size as `dest`. */
static void generate_normal_map(SDL_Surface* const src, SDL_Surface* const dest, const float intensity) {
	const int w = src -> w, h = src -> h;

	const SDL_PixelFormat* const dest_format = dest -> format;
	const float one_over_intensity = 1.0f / intensity;

	WITH_SURFACE_PIXEL_ACCESS(src,
		WITH_SURFACE_PIXEL_ACCESS(dest,

			for (int y = 0; y < h; y++) {
				sdl_pixel_t* dest_pixel = read_surface_pixel(dest, 0, y);

				for (int x = 0; x < w; x++, dest_pixel++) {
					const float
						tl = sobel_sample(src, x - 1, y - 1), tm = sobel_sample(src, x, y - 1),
						tr = sobel_sample(src, x + 1, y - 1), ml = sobel_sample(src, x - 1, y),
						mr = sobel_sample(src, x + 1, y), bl = sobel_sample(src, x - 1, y + 1),
						bm = sobel_sample(src, x, y + 1), br = sobel_sample(src, x + 1, y + 1);

					vec3 normal = {
						(-tl - ml * 2.0f - bl) + (tr + mr * 2.0f + br),
						(-tl - tm * 2.0f - tr) + (bl + bm * 2.0f + br),
						one_over_intensity
					};

					glm_vec3_normalize(normal);

					// Converting normal from range of (-1, 1) to (0, 1), and then to (0, `max_byte_value`)
					for (byte i = 0; i < 3; i++) normal[i] = (normal[i] * 0.5f + 0.5f) * constants.max_byte_value;

					*dest_pixel = SDL_MapRGB(dest_format,
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

static float* compute_1D_gaussian_kernel(const int radius, const float std_dev) {
	const int kernel_length = radius * 2 + 1;

	float* const kernel = malloc((size_t) kernel_length * sizeof(float)), sum = 0.0f;
	const float one_over_two_times_std_dev_squared = 1.0f / (2.0f * std_dev * std_dev);

	for (int x = 0; x < kernel_length; x++) {
		const int dx = x - radius;
		const float weight = expf(-(dx * dx) * one_over_two_times_std_dev_squared);
		kernel[x] = weight;
		sum += weight;
	}

	const float one_over_sum = 1.0f / sum;
	for (int i = 0; i < kernel_length; i++) kernel[i] *= one_over_sum;

	return kernel;
}

// It's assumed that `src` has the same size as `dest`.
static void do_separable_gaussian_blur_pass(SDL_Surface* const src,
	SDL_Surface* const dest, const float* const kernel,
	const int kernel_radius, const bool blur_is_vertical) {

	const int w = src -> w, h = src -> h;

	const SDL_PixelFormat
		*const src_format = src -> format,
		*const dest_format = dest -> format;

	const float one_over_max_byte_value = 1.0f / constants.max_byte_value;

	WITH_SURFACE_PIXEL_ACCESS(src,
		WITH_SURFACE_PIXEL_ACCESS(dest,

			for (int y = 0; y < h; y++) {
				sdl_pixel_t* dest_pixel = read_surface_pixel(dest, 0, y);

				for (int x = 0; x < w; x++, dest_pixel++) {
					float normalized_summed_channels[3] = {0.0f, 0.0f, 0.0f};

					for (int i = -kernel_radius; i <= kernel_radius; i++) {
						int filter_pos[2] = {x, y};
						filter_pos[blur_is_vertical] += i; // If blur is vertical, `blur_is_vertical` equals 1; otherwise, 0

						const sdl_pixel_t src_pixel = *(sdl_pixel_t*)
							edge_checked_read_surface_pixel(src, filter_pos[0], filter_pos[1]);

						sdl_pixel_component_t r, g, b;
						SDL_GetRGB(src_pixel, src_format, &r, &g, &b);

						const float one_over_max_byte_value_times_weight = one_over_max_byte_value * kernel[i + kernel_radius];
						normalized_summed_channels[0] += r * one_over_max_byte_value_times_weight;
						normalized_summed_channels[1] += g * one_over_max_byte_value_times_weight;
						normalized_summed_channels[2] += b * one_over_max_byte_value_times_weight;
					}

					*dest_pixel = SDL_MapRGB(dest_format,
						(sdl_pixel_component_t) (normalized_summed_channels[0] * constants.max_byte_value),
						(sdl_pixel_component_t) (normalized_summed_channels[1] * constants.max_byte_value),
						(sdl_pixel_component_t) (normalized_summed_channels[2] * constants.max_byte_value)
					);
				}
			}
		);
	);
}

GLuint init_normal_map_set_from_texture_set(const GLuint texture_set, const bool apply_blur) {
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

	set_current_texture(TexSet, texture_set);

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

	if (apply_blur) {
		const int blur_radius = constants.normal_mapping.blur.radius; // 1.2f
		float* const blur_kernel = compute_1D_gaussian_kernel(blur_radius, constants.normal_mapping.blur.std_dev);
		// Blurring #1 to #2 horizontally, and then blurring #2 vertically to #1
		do_separable_gaussian_blur_pass(general_purpose_surface_1, general_purpose_surface_2, blur_kernel, blur_radius, false);
		do_separable_gaussian_blur_pass(general_purpose_surface_2, general_purpose_surface_1, blur_kernel, blur_radius, true);
		free(blur_kernel);
	}

	// Making a normal map of #1 to #2
	generate_normal_map(general_purpose_surface_1, general_purpose_surface_2, constants.normal_mapping.intensity);

	////////// Making a new texture on the GPU, and then writing the normal map to that

	const GLuint normal_map_set = preinit_texture(TexSet,
		(TextureWrapMode) wrap_mode,
		(TextureFilterMode) mag_filter,
		(TextureFilterMode) min_filter);

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
