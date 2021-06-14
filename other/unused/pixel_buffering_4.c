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
		SDL_TEXTUREACCESS_STREAMING, screen_w, screen_h);
}

void deinit() {
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyTexture(texture);
	SDL_Quit();
}

void draw_pixel(int x, int y, int r, int g, int b) {
	Uint32* row = ((Uint32*) ((Uint8*) pixels + y * pitch)) + x;
	*row = 0xFF000000 | (r << 16) | (g << 8) | b;
}

int main() {
	init();
	SDL_Event event;

	while (1) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				deinit();
				return 0;
			}
		}

		SDL_LockTexture(texture, NULL, &pixels, &pitch);

		for (int x = 0; x < 20; x++)
			draw_pixel(x, screen_h / 2, 255, 0, 0);

		SDL_UnlockTexture(texture);

		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
		SDL_UpdateWindowSurface(window);
		SDL_Delay(10);
	}
}