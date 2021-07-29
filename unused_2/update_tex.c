#include <SDL2/SDL.h>
#include <sys/time.h>

static double time_in_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

int main(void) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Pixel formats" , 0, 0, 1920, 1080, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    double last_frame = time_in_ms();
    while (1) {
        int width, height;
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                return 0;
            }
        }
        SDL_GetWindowSize(window, &width, &height);
        SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);
        Uint32 *pixels = malloc(width * height * sizeof(*pixels));
        /* fill buffer with blue pixels */
        for (int y = 0; y < height; y++) {
            Uint32 *row = pixels + y * width;
            for (int x = 0; x < width; x++) {
                row[x] = 0x0000FFFF;
            }
        }

        double update_begin = time_in_ms();
        SDL_UpdateTexture(texture, NULL, pixels, width * sizeof(*pixels));
        double update_end = time_in_ms();
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        SDL_DestroyTexture(texture);
        free(pixels);
        double this_frame = time_in_ms();
        printf("frame took %fms\n", this_frame - last_frame);
        printf(" - update texture: %fms\n", update_end - update_begin);
        last_frame = this_frame;
    }

}
