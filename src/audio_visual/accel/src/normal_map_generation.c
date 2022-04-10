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

static void* read_surface_pixel(const SDL_Surface* const surface, const SDL_PixelFormat* const format, const int x, const int y) {
	sdl_pixel_component_t* const row = (sdl_pixel_component_t*) surface -> pixels + y * surface -> pitch;
	return row + x * format -> BytesPerPixel;
}

// If a coordinate (x or y) is out of bounds, it is converted to the closest possible edge value.
static void* edge_checked_read_surface_pixel(const SDL_Surface* const surface, const SDL_PixelFormat* const format, int x, int y) {
	x = limit_int_to_domain(x, 0, surface -> w - 1);
	y = limit_int_to_domain(y, 0, surface -> h - 1);

	return read_surface_pixel(surface, format, x, y);
}

static float sobel_sample(SDL_Surface* const surface,
	const SDL_PixelFormat* const format, const int x, const int y) {

	const sdl_pixel_t pixel = *(sdl_pixel_t*) edge_checked_read_surface_pixel(surface, format, x, y);

	sdl_pixel_component_t r, g, b;
	SDL_GetRGB(pixel, format, &r, &g, &b);

	// This equation is from https://en.wikipedia.org/wiki/Relative_luminance
	const float luminance = r * 0.2126f + g * 0.7152f + b * 0.0722f; // This ranges from 0 to `max_byte_value`
	return luminance / constants.max_byte_value; // Normalized from 0 to 1
}

/* This function is based on these sources:
- https://en.wikipedia.org/wiki/Sobel_operator
- https://www.shadertoy.com/view/Xtd3DS

Also, this function computes luminance values
of pixels to use those as heightmap values. */
static void generate_normal_map(SDL_Surface* const src, SDL_Surface* const normal_map, const float intensity) {
	const int src_w = src -> w, src_h = src -> h;

	const SDL_PixelFormat
		*const src_format = src -> format,
		*const dest_format = normal_map -> format;

	const float one_over_intensity = 1.0f / intensity;

	WITH_SURFACE_PIXEL_ACCESS(src,
		WITH_SURFACE_PIXEL_ACCESS(normal_map,

			for (int y = 0; y < src_h; y++) {
				for (int x = 0; x < src_w; x++) {
					const float
						tl = sobel_sample(src, src_format, x - 1, y - 1),
						tm = sobel_sample(src, src_format, x,     y - 1),
						tr = sobel_sample(src, src_format, x + 1, y - 1),

						ml = sobel_sample(src, src_format, x - 1, y),
						mr = sobel_sample(src, src_format, x + 1, y),

						bl = sobel_sample(src, src_format, x - 1, y + 1),
						bm = sobel_sample(src, src_format, x,     y + 1),
						br = sobel_sample(src, src_format, x + 1, y + 1);

					vec3 normal = {
						(-tl - ml * 2.0f - bl) + (tr + mr * 2.0f + br),
						(-tl - tm * 2.0f - tr) + (bl + bm * 2.0f + br),
						one_over_intensity
					};

					glm_vec3_normalize(normal);

					// Converting normal from range of (-1, 1) to (0, 1), and then to (0, `max_byte_value`)
					for (byte i = 0; i < 3; i++) normal[i] = (normal[i] * 0.5f + 0.5f) * constants.max_byte_value;

					const sdl_pixel_t normal_vector_in_rgb_format = SDL_MapRGB(
						dest_format,
						(sdl_pixel_component_t) normal[0],
						(sdl_pixel_component_t) normal[1],
						(sdl_pixel_component_t) normal[2]
					);

					*(sdl_pixel_t*) read_surface_pixel(normal_map, dest_format, x, y) = normal_vector_in_rgb_format;
				}
			}
		);
	);
}

////////// This code concerns Gaussian blur (the normal map input is blurred to cut out high frequencies from the Sobel operator).

static float* compute_1D_gaussian_kernel(const int radius, const float sigma) {
	const int kernel_length = radius * 2 + 1;

	float* const kernel = malloc((size_t) kernel_length * sizeof(float)), sum = 0.0f;
	const float one_over_two_times_sigma_squared = 1.0f / (2.0f * sigma * sigma);

	for (int x = 0; x < kernel_length; x++) {
		const int dx = x - radius;
		const float weight = expf(-(dx * dx) * one_over_two_times_sigma_squared);
		kernel[x] = weight;
		sum += weight;
	}

	const float one_over_sum = 1.0f / sum;
	for (int i = 0; i < kernel_length; i++) kernel[i] *= one_over_sum;

	return kernel;
}

// This function assumes that `src` and `dest` have the same size (which should always be the case).
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
				for (int x = 0; x < w; x++) {

					float normalized_summed_channels[3] = {0.0f, 0.0f, 0.0f};

					for (int i = -kernel_radius; i <= kernel_radius; i++) {

						int filter_pos[2] = {x, y};
						filter_pos[blur_is_vertical] += i; // If blur is vertical, `blur_is_vertical` equals 1; otherwise, 0

						const sdl_pixel_t pixel = *(sdl_pixel_t*)
							edge_checked_read_surface_pixel(src, src_format, filter_pos[0], filter_pos[1]);

						sdl_pixel_component_t r, g, b;
						SDL_GetRGB(pixel, src_format, &r, &g, &b);

						const float one_over_max_byte_value_times_weight = one_over_max_byte_value * kernel[i + kernel_radius];
						normalized_summed_channels[0] += r * one_over_max_byte_value_times_weight;
						normalized_summed_channels[1] += g * one_over_max_byte_value_times_weight;
						normalized_summed_channels[2] += b * one_over_max_byte_value_times_weight;
					}

					sdl_pixel_t* const dest_pixel = read_surface_pixel(dest, dest_format, x, y);

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

static GaussianBlurContext init_gaussian_blur_context(const float sigma,
	const int radius, const int blur_buffer_w, const int blur_buffer_h) {

	return (GaussianBlurContext) {
		.blur_buffer = {
			.horizontal = init_blank_surface(blur_buffer_w, blur_buffer_h, SDL_PIXEL_FORMAT),
			.size = {blur_buffer_w, blur_buffer_h}
		},

		.kernel = compute_1D_gaussian_kernel(radius, sigma),
		.kernel_radius = radius
	};
}

static void deinit_gaussian_blur_context(const GaussianBlurContext* const context) {
	deinit_surface(context -> blur_buffer.horizontal);
	free(context -> kernel);
}

static SDL_Surface* blur_surface(SDL_Surface* const src, const GaussianBlurContext context) {
	SDL_Surface
		*const horizontal_blur_buffer = context.blur_buffer.horizontal,
		*const vertical_blur_buffer = init_blank_surface(src -> w, src -> h, SDL_PIXEL_FORMAT);

	do_separable_gaussian_blur_pass(src, horizontal_blur_buffer, context.kernel, context.kernel_radius, 0);
	do_separable_gaussian_blur_pass(horizontal_blur_buffer, vertical_blur_buffer, context.kernel, context.kernel_radius, 1);

	return vertical_blur_buffer;
}

GLuint init_normal_map_set_from_texture_set(const GLuint texture_set) {
	/* How this function works:
	- First, query OpenGL about information about the texture set, like its dimensions, and its filters used.
	- Then, copy over its contents into a big surface.

	- Blur it via a two-pass Gaussian blur, and then generate a normal map from that.
		Note that the generated normal map is written into the original CPU-side surface, since that isn't used
		anymore at this point, and creating a new surface would be wasteful.

	- After that, reupload the normal map to the GPU via a new texture.

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

	const GLint surfaces_w = subtexture_w, surfaces_h = subtexture_h * num_subtextures;
	SDL_Surface* const cpu_side_surfaces = init_blank_surface(surfaces_w, surfaces_h, SDL_PIXEL_FORMAT);
	sdl_pixel_t* const cpu_side_surface_pixels = cpu_side_surfaces -> pixels;

	WITH_SURFACE_PIXEL_ACCESS(cpu_side_surfaces,
		glGetTexImage(TexSet, 0, OPENGL_INPUT_PIXEL_FORMAT,
			OPENGL_COLOR_CHANNEL_TYPE, cpu_side_surface_pixels);
	);

	////////// Blurring it, and then making a normal map out of it

	const GaussianBlurContext blur_context = init_gaussian_blur_context(
		constants.normal_mapping.blur.std_dev, constants.normal_mapping.blur.radius, surfaces_w, surfaces_h);

	SDL_Surface* const blurred = blur_surface(cpu_side_surfaces, blur_context);

	/* This reuses `cpu_side_surfaces` as a normal map, to save memory.
	`cpu_side_surfaces` is considered out of scope after this. */
	SDL_Surface* const normal_map = cpu_side_surfaces;
	generate_normal_map(blurred, normal_map, constants.normal_mapping.intensity);

	////////// Making a new texture on the GPU, and then writing the normal map into that

	const GLuint normal_map_set = preinit_texture(TexSet,
		(TextureWrapMode) wrap_mode,
		(TextureFilterMode) mag_filter,
		(TextureFilterMode) min_filter);

	WITH_SURFACE_PIXEL_ACCESS(normal_map,
		glTexImage3D(TexSet, 0, OPENGL_NORMAL_MAP_INTERNAL_PIXEL_FORMAT, subtexture_w,
			subtexture_h, num_subtextures, 0, OPENGL_INPUT_PIXEL_FORMAT,
			OPENGL_COLOR_CHANNEL_TYPE, normal_map -> pixels);
	);

	glGenerateMipmap(TexSet);

	////////// Deinitialization

	deinit_surface(normal_map);
	deinit_surface(blurred);
	deinit_gaussian_blur_context(&blur_context);

	return normal_map_set;
}

#endif
