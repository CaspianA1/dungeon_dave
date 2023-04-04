#ifndef MAP_UTILS_H
#define MAP_UTILS_H

#include "utils/typedefs.h" // For various typedefs
#include "glad/glad.h" // For OpenGL defs
#include "data/constants.h" // For `max_map_size`
#include "utils/failure.h" // For `FAIL`

////////// Indexed access to `map_pos_xz_t`

// This and the function below are not bounds-checked.
static inline map_pos_component_t get_indexed_map_pos_component(const map_pos_xz_t pos, const byte index) {
	return index ? pos.z : pos.x;
}

static inline map_pos_component_t* get_indexed_map_pos_component_ref(map_pos_xz_t* const pos, const byte index) {
	return index ? &pos -> z : &pos -> x;
}

////////// Map sampling

static inline map_pos_component_t sample_map(const Heightmap map, const map_pos_xz_t pos) {
	return map.data[pos.z * map.size.x + pos.x];
}

static inline map_texture_id_t sample_texture_id_map(
	const map_texture_id_t* const texture_id_map_data,
	const map_pos_component_t size_x, const map_pos_xz_t pos) {

	return texture_id_map_data[pos.z * size_x + pos.x];
}

////////// XZ bounds checking

static inline bool pos_out_of_overhead_map_bounds(const vec2 pos, const map_pos_xz_t size) {
	const GLfloat x = pos[0], z = pos[1];
	return (x < 0.0f) || (z < 0.0f) || (x >= size.x) || (z >= size.z);
}

////////// General map utilities

static inline map_pos_component_t get_heightmap_max_point_height(const Heightmap heightmap) {
	map_pos_component_t min = constants.max_map_size, max = 0;

	for (map_pos_component_t z = 0; z < heightmap.size.z; z++) {
		for (map_pos_component_t x = 0; x < heightmap.size.x; x++) {
			const map_pos_component_t height = sample_map(heightmap, (map_pos_xz_t) {x, z});
			if (height < min) min = height;
			if (height > max) max = height;
		}
	}

	if (min > 0) FAIL(UseLevelHeightmap, "Cannot use level heightmap because its smallest height, %u, is above 0\n", min);
	return max;
}

static inline GLfloat compute_world_far_clip_dist(const map_pos_xz_t map_size, const map_pos_component_t max_point_height) {
	/* The far clip distance, ideally, would be equal to the diameter of
	the convex hull of all points in the heightmap. If I had more time,
	I would implement that, but a simple method that works reasonably well is this:

	- First, find the smallest and tallest points in the map.
	- Then, the far clip distance equals the length of
		the `<map_width, map_height, max_point_height + additional_camera_height>` vector.

	To compute the maximum jump height, use the kinematics equation `v^2 = v0^2 + 2aΔy`.
	Given that `v` equals 0, rearrange the equation like this:

	0 = v0^2 + 2aΔy
	-(v0^2) = 2aΔy
	-(v0^2)/2a = Δy

	And since downward acceleration is positive in `constants`, to not get a negative result,
	remove the negative sign of the left term.

	Then, `additional_camera_height` equals `max_jump_height + eye_height`. */

	const GLfloat max_jump_height = (constants.speeds.jump * constants.speeds.jump) / (2.0f * constants.accel.g);
	const GLfloat additional_camera_height = max_jump_height + constants.camera.eye_height;

	return glm_vec3_norm((vec3) {map_size.x, max_point_height + additional_camera_height, map_size.z});
}

#endif
