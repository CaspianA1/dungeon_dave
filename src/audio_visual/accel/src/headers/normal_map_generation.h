#ifndef NORMAL_MAP_GENERATION_H
#define NORMAL_MAP_GENERATION_H

#include <SDL2/SDL.h>

/* Excluded: read_surface_pixel, edge_checked_read_surface_pixel,
sobel_sample, generate_normal_map, compute_1D_gaussian_kernel, do_separable_gaussian_blur_pass,
init_gaussian_blur_context, deinit_gaussian_blur_context, blur_surface */

/* The Gaussian blur performed is separable, so it needs a temporary surface for the horizontal pass.
TODO: make this struct private in some way from users of the .c file */
typedef struct {
    const struct {
        SDL_Surface* const horizontal;
        const int size[2];
    } blur_buffer;

    float* const kernel;
    const int kernel_radius;
} GaussianBlurContext;

GLuint init_normal_map_set_from_texture_set(const GLuint texture_set);

#endif
