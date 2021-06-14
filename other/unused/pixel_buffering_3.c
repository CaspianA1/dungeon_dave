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
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_STREAMING, screen_w, screen_h);
}

void deinit() {
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);
	SDL_Quit();
}

void draw_pixel(int x, int y, int r, int g, int b) {
	Uint32* row = ((Uint32*) ((Uint8*) pixels + y * pitch)) + x;
	*row = 0xFF000000 | (r << 16) | (g << 8) | b;
}

void draw_rectangle

/*
don't need to clear texture for raycaster; all good

needed texture operations:
- plot a pixel
- draw a fixed-color rectangle
- draw a scanline of a texture
*/

int main() {
	init();
	SDL_Event event;

	srand(time(NULL));

	int x = 0, y = 0, right = 1, down = 1, r = 10, g = 20, b = 40;

	while (1) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				deinit();
				return 0;
			}
		}

		x += right ? v : -v;
		y += down ? v : -v;

		if (x < 0) right = 1;
		else if (x > screen_w) right = 0, r = 80;

		if (y < 0) down = 1;
		else if (y > screen_h) down = 0, g = 40;

		SDL_LockTexture(texture, NULL, &pixels, &pitch);

		SDL_Rect square = {20, 20, 40, 40};
		SDL_RenderDrawRect(renderer, &square);

		/*
		for (int rx = 0; rx < 20; rx++) {
			for (int ry = 0; ry < 20; ry++)
				draw_pixel(rx + x, ry + y, r, g, b);
		}
		*/

		SDL_UnlockTexture(texture);

		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
		SDL_UpdateWindowSurface(window);
		SDL_Delay(10);
	}
}