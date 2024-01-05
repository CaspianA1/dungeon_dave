#ifndef AMBIENT_OCCLUSION_H
#define AMBIENT_OCCLUSION_H

/* This ambient occlusion implementation works by testing whether random rays from each point
in the heightmap (from the lowest to the highest point) intersect with any level geometry. */

#include "glad/glad.h" // For OpenGL defs
#include "utils/typedefs.h" // For various typedefs

//////////

/*
T = the total thread count
N = the number of trace iters per point
P = the number of points on the 3D grid
W = the workload split factor

- Normally, N loop iterations are run per thread, with each thread handling one point.
- But with this, N/W loop iterations are done per thread, with T*W total threads being spawned.
- This means that W threads correspond to one point.

Overall, having this split factor should save GPU time, by parallelizing
the workload more. It does use W times more memory though.
*/

typedef uint8_t workload_split_factor_t;

/* Note: this should be in sync
with `OPENGL_AO_MAP_INTERNAL_PIXEL_FORMAT`
and `OPENGL_AO_MAP_COLOR_CHANNEL_TYPE`,
in `texture.h`. */
typedef uint8_t ao_value_t;

typedef uint16_t trace_count_t;
typedef uint16_t ray_step_count_t;

typedef struct {
	/* TODO: why does increasing the workload split factor only make things slower?
	That doesn't make any sense. Also, see that this is nondestructive, in regards to
	divisions or moduli.*/
	const workload_split_factor_t workload_split_factor;
	const trace_count_t num_trace_iters;
	const ray_step_count_t max_num_ray_steps;
} AmbientOcclusionComputeConfig;

//////////

typedef struct {
	GLuint texture;
} AmbientOcclusionMap;

/* Excluded: generate_rand_dir, get_ao_term_from_collision_count, ray_collides_with_heightmap,
sign_between_map_values, clamp_signed_byte_to_directional_range, get_normal_data, transform_feedback_hook,
set_unpack_alignment, save_and_set_unpack_alignment, init_ao_map_texture */

AmbientOcclusionMap init_ao_map_with_copy_on_cpu(
	const Heightmap heightmap, const map_pos_component_t max_y,
	const AmbientOcclusionComputeConfig* const compute_config,
	ao_value_t** const cpu_copy); // `cpu_copy` should be freed with `dealloc` by the caller

AmbientOcclusionMap init_ao_map_from_cpu_copy(
	const Heightmap heightmap,
	const map_pos_component_t max_y,
	const ao_value_t* const cpu_data);

void deinit_ao_map(const AmbientOcclusionMap* const ao_map);

#endif
