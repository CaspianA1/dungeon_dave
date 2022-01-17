#include <SDL2/SDL.h>
#define PIXEL_FORMAT SDL_PIXELFORMAT_INDEX8

int main(void) {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Could not launch SDL: '%s'\n", SDL_GetError());
		return 1;
	}

	const int size[2] = {5, 5};

	SDL_Surface* const grayscale_surface = SDL_CreateRGBSurfaceWithFormat(0,
		size[0], size[1], SDL_BITSPERPIXEL(PIXEL_FORMAT), PIXEL_FORMAT);
	
	printf("%d\n", grayscale_surface -> pitch);
	
	SDL_LockSurface(grayscale_surface);
	Uint32* palette_index = grayscale_surface -> pixels;
	
	enum {num_colors = 256};
	SDL_Color palette[num_colors];
	for (int i = 0; i < num_colors; i++) palette[i] = (SDL_Color) {i, i, i, 255};
	SDL_SetPaletteColors(grayscale_surface -> format -> palette, palette, 0, num_colors);

	/* Padding?
	Or column major? */

	memset(palette_index, 127, grayscale_surface -> pitch * size[1] - 1);

	/*
	int count = 0;
	for (int y = 0; y < size[1]; y++) {
		for (int x = 0; x < size[0]; x++, palette_index++) {
			*palette_index = x * 20; //  / (float) y * 30.0f; //  & 255;
			count++;
		}
	}

	printf("%d\n", count);
	*/

	// memset(palette_index, 80, 3);
	
	SDL_SaveBMP(grayscale_surface, "out.bmp");
	SDL_FreeSurface(grayscale_surface);

	printf("Possible SDL error: '%s'\n", SDL_GetError());
	SDL_Quit();
}
