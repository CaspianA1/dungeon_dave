#ifndef AMBIENT_OCCLUSION_H
#define AMBIENT_OCCLUSION_H

/* This ambient occlusion implementation works by testing whether random rays from each point
in the heightmap (from the lowest to the highest point) intersect with any scene geometry */

#include "utils/typedefs.h" // For OpenGL types + other typedefs

typedef struct {
	const GLuint texture;
} AmbientOcclusionMap;

/* Excluded: generate_rand_dir, ray_collides_with_heightmap,
sign_between_bytes, clamp_signed_byte_to_directional_range, get_normal_data */

AmbientOcclusionMap init_ao_map(const byte* const heightmap, const byte map_size[2], const byte max_point_height);
void deinit_ao_map(const AmbientOcclusionMap* const ao_map);

#endif
