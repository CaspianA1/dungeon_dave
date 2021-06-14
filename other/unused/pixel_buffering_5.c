#include <SDL2/SDL.h>

const int screen_w = 400, screen_h = 600;

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* texture;
void* pixels;
int pitch;

const int v = 10;

void init() {
	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer(screen_w, screen_h, SDL_RENDERER_ACCELERATED, &window, &renderer);	
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_TARGET, screen_w, screen_h);
}

void deinit() {
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyTexture(texture);
	SDL_Quit();
}


void draw_rectangle(int r, int g, int b) {
	SDL_SetRenderDrawColor(renderer, r, g, b, 255);
	SDL_Rect test = {40, 40, 80, 80};
	SDL_RenderFillRect(renderer, &test);
	SDL_RenderDrawRect(renderer, &test);
}

int main() {
	init();
	SDL_Event event;

	/*
	SDL_SetRenderTarget(renderer, texture);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	*/

	SDL_SetRenderTarget(renderer, texture);

	while (1) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				deinit();
				return 0;
			}
		}
		
		draw_rectangle(255, 0, 0);

		SDL_SetRenderTarget(renderer, NULL);
		SDL_RenderCopy(renderer, texture, NULL, NULL);

		SDL_RenderPresent(renderer);
		SDL_UpdateWindowSurface(window);
		SDL_Delay(10);
	}
}