#ifndef NORMAL_MAP_GENERATION_C
#define NORMAL_MAP_GENERATION_C

#include "headers/normal_map_generation.h"
#include "texture.c"

// If a coordinate (x or y) is out of bounds, it is converted to the closest possible edge value.
inlinable void* read_surface_pixel(const SDL_Surface* const surface, const SDL_PixelFormat* const format, int x, int y) {
	const int w = surface -> w, h = surface -> h;

	if (x < 0) x = 0;
	else if (x >= w) x = w - 1;

	if (y < 0) y = 0;
	else if (y >= h) y = h - 1;

	Uint8* const row = (Uint8*) surface -> pixels + y * surface -> pitch;
	return row + x * format -> BytesPerPixel;
}

inlinable float sobel_sample(SDL_Surface* const surface,
	const SDL_PixelFormat* const format, const int x, const int y) {

	const Uint32 pixel = *(Uint32*) read_surface_pixel(surface, format, x, y);

	Uint8 r, g, b;
	SDL_GetRGB(pixel, format, &r, &g, &b);

	// Equation is from https://en.wikipedia.org/wiki/Relative_luminance
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

					// Converting normal from (-1, 1) range to (0, 1 range), and then to (0, 255 range)
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

#endif
