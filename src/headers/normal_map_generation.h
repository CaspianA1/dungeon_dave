#ifndef NORMAL_MAP_GENERATION_H
#define NORMAL_MAP_GENERATION_H

#include "buffer_defs.h"

/* Excluded:
int_min, int_max, limit_int_to_domain, sobel_sample,
generate_normal_map, compute_1D_gaussian_kernel,
do_separable_gaussian_blur_pass */

GLuint init_normal_map_set_from_texture_set(const GLuint texture_set, const bool apply_blur);

#endif
