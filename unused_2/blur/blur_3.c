// https://stackoverflow.com/questions/29521891/how-to-blur-sdl-surface-without-shaders
#include <SDL2/SDL.h>
#include <xmmintrin.h>

typedef uint_fast8_t byte;
typedef __v4si color;

#define PIXEL_FORMAT SDL_PIXELFORMAT_ARGB8888
#define DEBUG(var, format) printf(#var " = %" #format "\n", var)

Uint32 get_pixel(const SDL_Surface* const surface, const int x, const int y, const int bpp) {
	return *(Uint32*) ((Uint8*) surface -> pixels + y * surface -> pitch + x * bpp);
}

void put_pixel(SDL_Surface* const surface, const int x, const int y, const int bpp, const Uint32 pixel) {
 	*(Uint32*) ((Uint8*) surface -> pixels + y * surface -> pitch + x * bpp) = pixel;
}

void blur_image_portion(SDL_Surface* const image, SDL_Rect crop, const int blur_size) {
	const int src_w = image -> w, src_h = image -> h;
	SDL_Surface* const blurred_crop = SDL_CreateRGBSurfaceWithFormat(0, crop.w, crop.h, 32, PIXEL_FORMAT);
	SDL_LockSurface(blurred_crop);
	SDL_LockSurface(image);

	const SDL_PixelFormat* const format = image -> format;
	const int bpp = format -> BytesPerPixel;
 	for (int y = crop.y; y < crop.y + crop.h; y++) {
		for (int x = crop.x; x < crop.x + crop.w; x++) {
			color sum = {0, 0, 0, 0};
			int blur_sum_factor = 0;
			for (int py = -blur_size; py <= blur_size; py++) {
				for (int px = -blur_size; px <= blur_size; px++) {
					const int x1 = x + px, y1 = y + py;
					if (x1 < 0 || y1 < 0) continue;
					else if (x1 >= src_w || y1 >= src_h) break;

					const Uint32 pixel = get_pixel(image, x1, y1, bpp);

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
			put_pixel(blurred_crop, x - crop.x, y - crop.y, bpp, blurred_pixel);
		}
	}

	SDL_UnlockSurface(blurred_crop);
	SDL_UnlockSurface(image);
	SDL_BlitScaled(blurred_crop, NULL, image, &crop);
	SDL_FreeSurface(blurred_crop);
}

int main(int argc, char** argv) {
	if (argc != 2) {
		printf("Supply one argument, please\n");
		return 1;
	}

	// ../assets/walls/mesa.bmp
	// ../../assets/objects/doomguy.bmp
	// ../../assets/objects/bogo.bmp
	SDL_Surface* const unconverted_image = SDL_LoadBMP("../../assets/walls/rug_3.bmp");
	SDL_Surface* const image = SDL_ConvertSurfaceFormat(unconverted_image, PIXEL_FORMAT, 0);
	SDL_FreeSurface(unconverted_image);

	blur_image_portion(image, (SDL_Rect) {0, 0, image -> w, image -> h / 2}, atoi(argv[1]));
	SDL_SaveBMP(image, "out.bmp");
	SDL_FreeSurface(image);
}
