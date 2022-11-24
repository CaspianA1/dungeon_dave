#ifndef CAMERA_H
#define CAMERA_H

#include "data/constants.h" // For various constant defs
#include "utils/typedefs.h" // For OpenGL types + other typedefs
#include "utils/cglm_include.h" // For `vec2, `vec3, `mat4`, and `vec4`
#include "event.h" // For `Event`

// TODO: use the yaw, pitch, and roll nomenclature

//////////

// These are the Euler angles
typedef struct {GLfloat hori, vert, tilt;} Angles;

typedef struct {
	const vec3 init_pos;
	const Angles angles;
} CameraConfig;

typedef struct {
	Angles angles;

	/* Pace is the amount of head bob that happens when moving.
	The speed xz percent is not the true speed percent; rather,
	the percentage is smoothed out by a Hermite curve. */
	GLfloat
		fov, pace, speed_xz_percent, time_since_jump,
		time_accum_for_full_fov, far_clip_dist;

	vec2 right_xz; // This is used for billboards
	vec3 pos, dir, right, up, velocities;

	mat4 view, view_projection;
	vec4 frustum_planes[planes_per_frustum];
} Camera;

/* Excluded:
Utils: clamp_to_pos_neg_domain, wrap_around_domain, get_percent_kept_from, smootherstep
Angle updating: update_camera_angles, update_fov
Physics + collision: apply_velocity_in_xz_direction, tile_exists_at_pos, pos_collides_with_heightmap, update_pos_via_physics
Pace: make_pace_function, update_pace
Miscellaneous: get_camera_directions, update_camera_pos, update_camera_matrices */

void update_camera(Camera* const camera, const Event* const event, const byte* const heightmap, const byte map_size[2]);
Camera init_camera(const CameraConfig* const config, const GLfloat far_clip_dist);

#endif
