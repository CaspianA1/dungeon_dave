#ifndef NORMAL_MAP_GENERATION_H
#define NORMAL_MAP_GENERATION_H

#include "utils/buffer_defs.h"

/* Excluded:
int_min, int_max, limit_int_to_domain, sobel_sample,
generate_normal_map, compute_1D_gaussian_kernel,
do_separable_gaussian_blur_pass */

typedef struct {
    const signed_byte blur_radius; // This can be zero. If so, no blurring happens.
    const GLfloat blur_std_dev, intensity;
} NormalMapConfig;

GLuint init_normal_map_from_diffuse_texture(const GLuint diffuse_texture, const NormalMapConfig* const config);

#endif
