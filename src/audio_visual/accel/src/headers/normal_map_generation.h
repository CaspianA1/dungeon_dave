#ifndef NORMAL_MAP_GENERATION_H
#define NORMAL_MAP_GENERATION_H

#include <SDL2/SDL.h>

/* Excluded: read_surface_pixel, edge_checked_read_surface_pixel,
sobel_sample, compute_1D_gaussian_kernel, do_separable_gaussian_blur_pass */

// The Gaussian blur performed is separable, so it needs a temporary surface for the horizontal pass
typedef struct {
    const struct {
        SDL_Surface* const horizontal;
        const int size[2];
    } blur_buffer;

    float* const kernel;
    const int kernel_radius;
} GaussianBlurContext;

SDL_Surface* generate_normal_map(SDL_Surface* const src_image, const float intensity);

GaussianBlurContext init_gaussian_blur_context(const float sigma,
	const int radius, const int blur_buffer_w, const int blur_buffer_h);

void deinit_gaussian_blur_context(const GaussianBlurContext* const context);

SDL_Surface* blur_surface(const SDL_Surface* const src, const GaussianBlurContext context);

void init_texture_set_with_adjacent_normal_maps(const GLsizei num_src_textures,
	const GLsizei rescale_w, const GLsizei rescale_h, ...);

#endif
