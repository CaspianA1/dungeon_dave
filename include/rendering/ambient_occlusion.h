#ifndef AMBIENT_OCCLUSION_H
#define AMBIENT_OCCLUSION_H

/* This ambient occlusion implementation works by testing whether random rays from each point
in the heightmap (from the lowest to the highest point) intersect with any scene geometry */

#include "utils/buffer_defs.h"
#include "utils/opengl_wrappers.h"

typedef GLuint AmbientOcclusionMap;

// Excluded: generate_rand_number_within_range, generate_random_dir, ray_collides_with_heightmap

AmbientOcclusionMap init_ao_map(const byte* const heightmap, const byte map_size[2], const byte max_point_height);
#define deinit_ao_map deinit_texture

#endif
