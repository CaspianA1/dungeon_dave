#ifndef NORMAL_MAP_GENERATION_H
#define NORMAL_MAP_GENERATION_H

#include "utils.h"

// Excluded: read_surface_pixel, sobel_sample, get_3x3_kernel_from_surface

SDL_Surface* generate_normal_map(SDL_Surface* const src_image, const float intensity);

#endif
