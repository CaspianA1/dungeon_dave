// https://stackoverflow.com/questions/29521891/how-to-blur-sdl-surface-without-shaders
#include <SDL2/SDL.h>
#include <x86intrin.h>

typedef uint_fast8_t byte;
typedef __v4si color;

#define PIXEL_FORMAT SDL_PIXELFORMAT_ARGB8888
#define DEBUG(var, format) printf(#var " = %" #format "\n", var)

Uint32 get_pixel(const SDL_Surface* const surface, const int x, const int y) {
	const int bpp = surface ->format -> BytesPerPixel;
	return *(Uint32*) ((Uint8*) surface->pixels + y * surface -> pitch + x * bpp);
}

void put_pixel(SDL_Surface* const surface, const int x, const int y, const Uint32 pixel) {
 	const int bpp = surface -> format-> BytesPerPixel;
 	*(Uint32*) ((Uint8*) surface->pixels + y * surface -> pitch + x * bpp) = pixel;
}

byte get_bits(const Uint32 value, const byte offset, const byte n) {
	return (value >> offset) & ((1 << n) - 1);
}

SDL_Surface* blur_image_portion(const SDL_Surface* const src, const SDL_Rect crop, const int blur_size) {
	const int src_w = src -> w, src_h = src -> h;
	SDL_Surface* const dest = SDL_CreateRGBSurfaceWithFormat(0, src_w, src_h, 32, PIXEL_FORMAT);

	const SDL_PixelFormat* const format = src -> format;
	const int bpp = src -> format -> BytesPerPixel;
 	for (int y = crop.y; y < crop.y + crop.h; y++) {
		for (int x = crop.x; x < crop.x + crop.w; x++) {
			color sum = {0, 0, 0, 0};
			int blur_sum_factor = 0;
			for (int py = -blur_size; py <= blur_size; py++) {
				for (int px = -blur_size; px <= blur_size; px++) {
					const int x1 = x + px, y1 = y + py;
					if (x1 < 0 || y1 < 0) continue;
					else if (x1 >= src_w || y1 >= src_h) break;

					const Uint32 pixel = get_pixel(src, x1, y1);

					byte r, g, b, a;
					SDL_GetRGBA(pixel, format, &r, &g, &b, &a);
					sum += (color) {r, g, b, a};
	
					blur_sum_factor++;
                }
			}

			const byte out[4] = {
				sum[0] / blur_sum_factor,
				sum[1] / blur_sum_factor,
				sum[2] / blur_sum_factor,
				sum[3] / blur_sum_factor
			};

			const Uint32 blurred_pixel = SDL_MapRGBA(format, out[0], out[1], out[2], out[3]);
			put_pixel(dest, x, y, blurred_pixel);
		}
	}
	return dest;
}

int main(void) {
	// ../assets/walls/mesa.bmp
	// ../../assets/objects/doomguy.bmp
	// ../../assets/objects/bogo.bmp
	SDL_Surface* const unconverted_src = SDL_LoadBMP("../../assets/walls/sand.bmp");
	SDL_Surface* const src = SDL_ConvertSurfaceFormat(unconverted_src, PIXEL_FORMAT, 0);
	SDL_Surface* const blurred = blur_image_portion(src, (SDL_Rect) {0, 0, src -> w, src -> h / 2}, 4);
	SDL_SaveBMP(blurred, "out.bmp");

	SDL_FreeSurface(unconverted_src);
	SDL_FreeSurface(src);
	SDL_FreeSurface(blurred);
}
