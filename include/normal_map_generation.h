#ifndef NORMAL_MAP_GENERATION_H
#define NORMAL_MAP_GENERATION_H

#include "utils/buffer_defs.h"
#include "utils/texture.h"

/* Excluded:
int_min, int_max, limit_int_to_domain, sobel_sample,
generate_normal_map, compute_1D_gaussian_kernel,
do_separable_gaussian_blur_pass, get_texture_metadata */

typedef struct {
    const signed_byte blur_radius; // This can be zero. If so, no blurring happens.
    const GLfloat blur_std_dev, intensity, rescale_factor;
} NormalMapConfig;

// This bakes an inverted heightmap into the alpha channel of the normal map too, for use with parallax mapping
GLuint init_normal_map_from_diffuse_texture(const GLuint diffuse_texture,
	const TextureType type, const NormalMapConfig* const config);

#endif
