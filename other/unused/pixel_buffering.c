/*
https://gamedev.stackexchange.com/questions/102490/fastest-way-to-render-image-data-from-buffer
https://wiki.libsdl.org/SDL_CreateTexture
https://wiki.libsdl.org/SDL_PixelFormat
https://wiki.libsdl.org/SDL_PixelFormatEnum
https://wiki.libsdl.org/SDL_LockTexture
https://wiki.libsdl.org/SDL_UnlockTexture
https://gamedev.stackexchange.com/questions/175341/what-do-i-provide-to-sdl-locktexture-pixels
https://www.google.com/search?q=sdl+texture+lock+explained&oq=sdl+texture+lock+explained&aqs=chrome..69i57.5033j0j7&sourceid=chrome&ie=UTF-8
https://gamedev.stackexchange.com/questions/98641/how-do-i-modify-textures-in-sdl-with-direct-pixel-access
https://www.google.com/search?q=generate+24-bit+color+from+rgb+values&oq=generate+24-bit+color+from+rgb+values&aqs=chrome..69i57.7265j0j7&sourceid=chrome&ie=UTF-8
https://gamedev.stackexchange.com/questions/98641/how-do-i-modify-textures-in-sdl-with-direct-pixel-access
https://gamedev.stackexchange.com/questions/175341/what-do-i-provide-to-sdl-locktexture-pixels
https://github.com/libsdl-org/SDL/blob/e2fd1c0fe39d83ecdee1e0c6359a299f0afad13b/test/teststreaming.c
*/

#include <SDL2/SDL.h>

const int screen_w = 600, screen_h = 800;

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* screen_texture;
int** pixel_buffer;
int pitch;

void init() {
	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer(screen_w, screen_h, SDL_RENDERER_ACCELERATED, &window, &renderer);

	screen_texture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_STREAMING,
		screen_w, screen_h);

	pixel_buffer = calloc(screen_h, sizeof(void*));
	for (int y = 0; y < screen_h; y++)
		pixel_buffer[y] = calloc(screen_w, sizeof(void));

	pitch = screen_w * sizeof(int);
}

void set_pixel(int x, int y, int r, int g, int b) {
	/*
	set a byte to {r, g, b, 255}
	but how?
	not possible
	need 32 bits (including the 'a' component)
	*/
}

void deinit() {
	printf("Win\n");
	SDL_DestroyWindow(window);
	printf("Rend\n");
	SDL_DestroyRenderer(renderer);

	printf("Tex\n");
	SDL_DestroyTexture(screen_texture);
	SDL_Quit();

	/*
	for (int y = 0; y < screen_h; y++)
		free(pixel_buffer[y]);
	free(pixel_buffer);
	*/
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

			else printf("Other\n");
		}

		printf("%d\n", (int) pixel_buffer[0][0]);
		SDL_LockTexture(screen_texture, NULL, (void*) pixel_buffer, &pitch);
		SDL_UnlockTexture(screen_texture);

		SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
		SDL_RenderPresent(renderer);
		SDL_UpdateWindowSurface(window);
		SDL_Delay(100);
	}
}