#ifndef NORMAL_MAP_GENERATION_C
#define NORMAL_MAP_GENERATION_C

#include "headers/normal_map_generation.h"
#include "headers/constants.h"
#include "texture.c"

static void* read_surface_pixel(const SDL_Surface* const surface, const SDL_PixelFormat* const format, const int x, const int y) {
	Uint8* const row = (Uint8*) surface -> pixels + y * surface -> pitch;
	return row + x * format -> BytesPerPixel;
}

// If a coordinate (x or y) is out of bounds, it is converted to the closest possible edge value.
static void* edge_checked_read_surface_pixel(const SDL_Surface* const surface, const SDL_PixelFormat* const format, int x, int y) {
	const int w = surface -> w, h = surface -> h;

	if (x < 0) x = 0;
	else if (x >= w) x = w - 1;

	if (y < 0) y = 0;
	else if (y >= h) y = h - 1;

	return read_surface_pixel(surface, format, x, y);
}

static float sobel_sample(SDL_Surface* const surface,
	const SDL_PixelFormat* const format, const int x, const int y) {

	const Uint32 pixel = *(Uint32*) edge_checked_read_surface_pixel(surface, format, x, y);

	Uint8 r, g, b;
	SDL_GetRGB(pixel, format, &r, &g, &b);

	// This equation is from https://en.wikipedia.org/wiki/Relative_luminance
	const float luminance = r * 0.2126f + g * 0.7152f + b * 0.0722f;
	return luminance / 255.0f; // Normalized from 0 to 1
}

/* This function is based on these sources:
- https://en.wikipedia.org/wiki/Sobel_operator
- https://www.shadertoy.com/view/Xtd3DS

Also, this function computes luminance values
of pixels to use those as heightmap values. */
SDL_Surface* generate_normal_map(SDL_Surface* const src, const float intensity) {
	const int src_w = src -> w, src_h = src -> h;

	SDL_Surface* const normal_map = init_blank_surface(src_w, src_h);

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

					const float
						gx = (-tl - ml * 2.0f - bl) + (tr + mr * 2.0f + br),
						gy = (-tl - tm * 2.0f - tr) + (bl + bm * 2.0f + br);

					vec3 normal = {gx, gy, one_over_intensity};
					glm_vec3_normalize(normal);

					// Converting normal from range of (-1, 1) to (0, 1), and then to (0, 255)
					for (byte i = 0; i < 3; i++) normal[i] = (normal[i] * 0.5f + 0.5f) * 255.0f;

					const Uint32 normal_vector_in_rgb_format = SDL_MapRGB(
						dest_format, (Uint8) normal[0], (Uint8) normal[1], (Uint8) normal[2]
					);

					*(Uint32*) read_surface_pixel(normal_map, dest_format, x, y) = normal_vector_in_rgb_format;
				}
			}
		);
	);

	return normal_map;
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
static void do_separable_gaussian_blur_pass(const SDL_Surface* const src,
	SDL_Surface* const dest, const float* const kernel,
	const int kernel_radius, const bool blur_is_vertical) {

	const int w = src -> w, h = src -> h;

	const SDL_PixelFormat
		*const src_format = src -> format,
		*const dest_format = dest -> format;

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {

			float normalized_summed_channels[3] = {0.0f, 0.0f, 0.0f};
			for (int i = -kernel_radius; i <= kernel_radius; i++) {

				int filter_pos[2] = {x, y};
				filter_pos[blur_is_vertical] += i; // If blur is vertical, `blur_is_vertical` equals 1; otherwise, 0

				const Uint32 pixel = *(Uint32*) edge_checked_read_surface_pixel(src, src_format, filter_pos[0], filter_pos[1]);

				Uint8 r, g, b;
				SDL_GetRGB(pixel, src_format, &r, &g, &b);

				const float weight = kernel[i + kernel_radius];
				normalized_summed_channels[0] += (r / 255.0f) * weight;
				normalized_summed_channels[1] += (g / 255.0f) * weight;
				normalized_summed_channels[2] += (b / 255.0f) * weight;
			}

			Uint32* const dest_pixel = read_surface_pixel(dest, dest_format, x, y);

			*dest_pixel = SDL_MapRGB(dest_format,
				(Uint8) (normalized_summed_channels[0] * 255.0f),
				(Uint8) (normalized_summed_channels[1] * 255.0f),
				(Uint8) (normalized_summed_channels[2] * 255.0f)
			);
		}
	}
}

GaussianBlurContext init_gaussian_blur_context(const float sigma,
	const int radius, const int blur_buffer_w, const int blur_buffer_h) {

	return (GaussianBlurContext) {
		.blur_buffer = {
			.horizontal = init_blank_surface(blur_buffer_w, blur_buffer_h),
			.size = {blur_buffer_w, blur_buffer_h}
		},

		.kernel = compute_1D_gaussian_kernel(radius, sigma),
		.kernel_radius = radius
	};
}

void deinit_gaussian_blur_context(const GaussianBlurContext* const context) {
	deinit_surface(context -> blur_buffer.horizontal);
	free(context -> kernel);
}

SDL_Surface* blur_surface(const SDL_Surface* const src, const GaussianBlurContext context) {
	SDL_Surface
		*const horizontal_blur_buffer = context.blur_buffer.horizontal,
		*const vertical_blur_buffer = init_blank_surface(src -> w, src -> h);

	do_separable_gaussian_blur_pass(src, horizontal_blur_buffer, context.kernel, context.kernel_radius, 0);
	do_separable_gaussian_blur_pass(horizontal_blur_buffer, vertical_blur_buffer, context.kernel, context.kernel_radius, 1);

	return vertical_blur_buffer;
}

// TODO: remove
void test_normal_map_generation(void) {
	const int rescale_w = 256, rescale_h = 256, blur_radius = 5;
	const float blur_std_deviation = 3.5f, normal_map_intensity = 1.1f;

	////////// Testing

	SDL_Surface* const test_surface = init_surface("../../../../assets/walls/pyramid_bricks_4.bmp");
	SDL_Surface* const upscaled_test_surface = init_blank_surface(rescale_w, rescale_h);
	SDL_BlitScaled(test_surface, NULL, upscaled_test_surface, NULL);

	const GaussianBlurContext gaussian_blur_context = init_gaussian_blur_context(
		blur_std_deviation, blur_radius, rescale_w, rescale_h);

	SDL_Surface* const blurred_test_surface = blur_surface(upscaled_test_surface, gaussian_blur_context);
	SDL_Surface* const normal_map_of_surface = generate_normal_map(blurred_test_surface, normal_map_intensity);

	SDL_SaveBMP(normal_map_of_surface, "out.bmp");

	////////// Deinitialization

	deinit_surface(normal_map_of_surface);
	deinit_surface(blurred_test_surface);

	deinit_gaussian_blur_context(&gaussian_blur_context);

	deinit_surface(upscaled_test_surface);
	deinit_surface(test_surface);
}

#endif