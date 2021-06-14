#include <SDL2/SDL.h>

// goal: write a matrix of pixels to the screen effeciently

const int screen_w = 400, screen_h = 300;

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* texture;

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

void update_texture(int x, int y, int r, int g, int b) {
	void *pixels;
	int pitch;

	int color = 0xFF000000 | (r << 16) | (g << 8) | b;

	int a = SDL_LockTexture(texture, NULL, &pixels, &pitch);

	Uint32* pixel = (Uint32*) (pixels + pitch * x);
	*pixel = color;

	/*
	for (int row = 0; row < screen_h; row++) {
	    Uint32* dst = (Uint32*) ((Uint8*) pixels + row * pitch);
	    for (int col = 0; col < screen_w; col++)
	        *dst++ = color;
	}
	*/

	SDL_UnlockTexture(texture);
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

		int color = 0;
		for (int x = 0; x < screen_w; x++) {
			for (int y = 0; y < screen_h; y++) {
				update_texture(x, y, color, color, color);
				if (++color > 255) color = 0;
			}
		}

		// update_texture(3, 3, 255, 0, 0);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
		SDL_UpdateWindowSurface(window);
		SDL_Delay(100);
	}
}

/*
void* pixels;
int pitch;

SDL_Color tan_color = {210, 180, 140, 255};

SDL_LockTexture(texture, NULL, &pixels, &pitch);

for (int x = 0; x < screen_w; x++) {
	Uint32* dst = (Uint32*) ((Uint8*) pixels + x * pitch);
	for (int y = 0; y < screen_h; y++) {
		*dst++ = 0xFF000000 | (tan_color.r << 16) | (tan_color.g << 8) | tan_color.b;
	}
}
SDL_UnlockTexture(texture);

/////

void update_texture(int pixel_x, int pixel_y, int r, int g, int b);

// Uint32* pixel = (Uint32*) (pixels + pitch * x);
// Uint32* pixel = (Uint32*) ((Uint8*) pixels + (y * pitch + x) + x);
*/