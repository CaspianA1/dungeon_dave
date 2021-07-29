#include <SDL2/SDL.h>

enum {width = 640, height = 480};

int main(void) {
	SDL_Init(SDL_INIT_VIDEO);

	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_CreateWindowAndRenderer(width, height, SDL_RENDERER_ACCELERATED | SDL_WINDOW_RESIZABLE, &window, &renderer);

	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);

	Uint32* pixels = malloc(width * height * sizeof(Uint32));
	for (int y = 0; y < height; y++) {
	    Uint32* row = pixels + y * width;
	    for (int x = 0; x < width; x++) {
	        row[x] = 0x0000FFFF;
	    }
	}

	SDL_UpdateTexture(texture, NULL, pixels, width * sizeof(Uint32));

	SDL_Event event;
	while (1) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				free(pixels);
				SDL_DestroyTexture(texture);
				SDL_Quit();
				return 0;
			}
		}
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
	}
}
