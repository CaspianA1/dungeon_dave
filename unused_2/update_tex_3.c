#include <SDL2/SDL.h>

enum {width = 640, height = 480, format = SDL_PIXELFORMAT_RGBA8888};

SDL_Surface* get_surface(const char* path) {
	SDL_Surface* unconverted = SDL_LoadBMP(path);
	SDL_Surface* converted = SDL_ConvertSurfaceFormat(unconverted, format, 0);
	SDL_FreeSurface(unconverted);
	return converted;
}

int main(void) {
	SDL_Init(SDL_INIT_VIDEO);

	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_CreateWindowAndRenderer(width, height, SDL_RENDERER_ACCELERATED | SDL_WINDOW_RESIZABLE, &window, &renderer);

	SDL_Surface* surface = get_surface("../assets/walls/dirt.bmp");
	SDL_Texture* texture = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_STREAMING, surface -> w, surface -> h);

	SDL_UpdateTexture(texture, NULL, surface -> pixels, surface -> w * sizeof(Uint32));
	SDL_Event event;
	while (1) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				SDL_FreeSurface(surface);
				SDL_DestroyTexture(texture);
				SDL_Quit();
				return 0;
			}
		}
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
	}
}
