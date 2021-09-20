#include <SDL2/SDL.h>

#define FAIL(...) {fprintf(stderr, __VA_ARGS__ "\n"); exit(1);}
typedef uint_fast8_t byte;
enum {w = 800, h = 600};

int main(void) {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) FAIL("Could not initialize SDL");

	SDL_Window* window;
	SDL_Renderer* renderer;
	if (SDL_CreateWindowAndRenderer(w, h, SDL_RENDERER_ACCELERATED | SDL_WINDOW_RESIZABLE, &window, &renderer) < 0)
		FAIL("Could not create a window or renderer");

	byte running = 1;
	SDL_Event event;
	while (running) {
		// SDL_RenderClear(renderer);
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) running = 0;
		}

		SDL_RenderPresent(renderer);
	}

	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);
	SDL_Quit();
}
