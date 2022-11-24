#ifndef MAP_UTILS_H
#define MAP_UTILS_H

#include "utils/typedefs.h" // For OpenGL types + other typedefs
#include "utils/failure.h" // For `FAIL`

static inline byte sample_map_point(const byte* const map, const byte x, const byte z, const byte map_width) {
	return map[z * map_width + x];
}

static inline bool pos_out_of_overhead_map_bounds(const GLfloat x,
	const GLfloat z, const byte map_width, const byte map_height) {

	return (x < 0.0f) || (z < 0.0f) || (x >= map_width) || (z >= map_height);
}

static inline byte get_heightmap_max_point_height(const byte* const heightmap, const byte map_size[2]) {
	const byte map_width = map_size[0], map_height = map_size[1];

	byte min = constants.max_byte_value, max = 0;

	for (byte y = 0; y < map_height; y++) {
		for (byte x = 0; x < map_width; x++) {
			const byte height = sample_map_point(heightmap, x, y, map_width);
			if (height < min) min = height;
			if (height > max) max = height;
		}
	}

	if (min > 0) FAIL(UseLevelHeightmap, "Cannot use level heightmap because its smallest height, %u, is above 0\n", min);
	return max;
}

static inline GLfloat compute_world_far_clip_dist(const byte map_size[2], const byte max_point_height) {
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
	return glm_vec3_norm((vec3) {map_size[0], map_size[1], max_point_height + additional_camera_height});
}

#endif
