// https://stackoverflow.com/questions/29521891/how-to-blur-sdl-surface-without-shaders
#include <SDL2/SDL.h>

typedef uint_fast8_t byte;

#define PIXEL_FORMAT SDL_PIXELFORMAT_ARGB8888

Uint32 get_pixel(const SDL_Surface* const surface, const int x, const int y) {
	const int bpp = surface ->format -> BytesPerPixel;
	return *(Uint32*) ((Uint8 *)surface->pixels + y * surface -> pitch + x * bpp);
}


void put_pixel(SDL_Surface* const surface, const int x, const int y, const Uint32 pixel) {
 	const int bpp = surface -> format-> BytesPerPixel;
 	*(Uint32*) ((Uint8 *)surface->pixels + y * surface -> pitch + x * bpp) = pixel;
}


void blurrer(const char* path, const int blur_size) {
	const SDL_Surface* const sfc_blur = SDL_ConvertSurfaceFormat(SDL_LoadBMP(path), PIXEL_FORMAT, 0);
	SDL_Surface* const sfc_blur_target = SDL_CreateRGBSurfaceWithFormat(0, sfc_blur -> w, sfc_blur -> h, 32, PIXEL_FORMAT);

	const int img_w = sfc_blur -> w, img_h = sfc_blur -> h;

 	for (int y = 0; y < img_h; y++) {
		for (int x = 0; x < img_w; x++) {
			Uint64 sum[3] = {0, 0, 0};
			int blur_sum_factor = 0;
			for (int py = -blur_size; py <= blur_size; py++) {
				for (int px = -blur_size; px <= blur_size; px++) {
					const int x1 = x + px, y1 = y + py;
					if (x1 < 0 || y1 < 0) continue;
					else if (x1 >= img_w || y1 >= img_h) break;

					const Uint32 pixel = get_pixel(sfc_blur, x1, y1);
					byte r, g, b;
					SDL_GetRGB(pixel, sfc_blur -> format, &r, &g, &b);
					sum[0] += r;
					sum[1] += g;
					sum[2] += b;
					blur_sum_factor++;
                }
			}
			const byte out[3] = {
				sum[0] / blur_sum_factor,
				sum[1] / blur_sum_factor,
				sum[2] / blur_sum_factor
			};

			// const Uint32 center = get_pixel(sfc_blur, x, y);
			// const Uint32 alpha = (get_pixel(sfc_blur, x, y) & 0xFF000000) >> 31;
			// const Uint32 alpha = 0xFF000000;
			// how to extract the alpha channel?

			const Uint32 blurred_pixel = 0xFF000000 | (out[0] << 16) | (out [1] << 8) | out[2];
			put_pixel(sfc_blur_target, x, y, blurred_pixel);
		}
	}
    SDL_SaveBMP(sfc_blur_target, "out.bmp");
}

int main(void) {
	// ../assets/walls/mesa.bmp
	// ../../assets/objects/doomguy.bmp
	// ../../assets/objects/bogo.bmp
    blurrer("../../assets/objects/bogo.bmp", 3);
}
