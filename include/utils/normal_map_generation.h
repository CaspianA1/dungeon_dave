#ifndef NORMAL_MAP_GENERATION_H
#define NORMAL_MAP_GENERATION_H

#include "utils/typedefs.h" // For various typedefs
#include "glad/glad.h" // For OpenGL defs
#include "utils/texture.h" // For `TextureType`

/* Excluded:
generate_heightmap, int_min, int_max, int_clamp, sobel_sample,
generate_normal_map, compute_1D_gaussian_kernel,
do_separable_gaussian_blur_pass, get_texture_metadata */

typedef struct {
	const byte blur_radius; // This can be zero. If so, no blurring happens.
	const GLfloat blur_std_dev, heightmap_scale, rescale_factor;
} NormalMapConfig;

typedef struct {
	const GLuint shader; // TODO: use this
} NormalMapCreator;

// This bakes an inverted heightmap into the alpha channel of the normal map too, for use with parallax mapping
GLuint init_normal_map_from_albedo_texture(
	const NormalMapCreator* const creator,
	const NormalMapConfig* const config,
	const GLuint albedo_texture, const TextureType type);

//////////

NormalMapCreator init_normal_map_creator(void);
void deinit_normal_map_creator(const NormalMapCreator* const creator);

#endif
