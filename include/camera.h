#ifndef CAMERA_H
#define CAMERA_H

#include "data/constants.h" // For various constant defs
#include "utils/typedefs.h" // For OpenGL types + other typedefs
#include "cglm/cglm.h" // For `vec2, `vec3, `mat4`, and `vec4`
#include "event.h" // For `Event`
#include "openal/al.h" // For various OpenAL defs

// TODO: make direction switching harder (maybe), and apply wall alignment slowdown for corners

//////////

// These are the Euler angles
typedef struct {GLfloat hori, vert, tilt;} Angles;

typedef struct {
	const vec3 init_pos;
	const Angles angles;
} CameraConfig;

// TODO: separate out physics and the camera
typedef struct {
	Angles angles;

	/* Pace is the amount of head bob that happens when moving.
	The speed xz percent is not the true speed percent; rather,
	the percentage is smoothed out by a Hermite curve.

	Also note that `jump_fall_velocity` is not necessarily equal to
	`velocity_world_space[1].` `jump_fall_velocity` only accounts for
	jumping/falling, and not the pace.

	Also, the xz view-space velocity does not account for slowdown. It is more
	of an indication of how much the player is trying to move in any direction. */

	bool landed_jump_in_this_tick;

	GLfloat
		fov, pace, speed_xz_percent, last_tick_wall_alignment_percent,
		time_since_jump, time_accum_for_full_fov, jump_fall_velocity, far_clip_dist;

	vec2 velocity_xz_view_space, right_xz; // This is used for billboards
	vec3 pos, dir, right, up, velocity_world_space;

	mat4 view, view_projection;
	vec4 frustum_planes[planes_per_frustum];
} Camera;

/* Excluded:
Utils: clamp_to_pos_neg_domain, clamp_vec2_to_pos_neg_domain, wrap_around_domain, get_percent_kept_from, smootherstep
Angle updating: update_camera_angles, update_fov
Physics + collision: get_pos_collision_info, get_aabb_collision_info, update_pos
Pace: make_pace_function, update_pace
Miscellaneous: get_camera_directions, update_camera_pos, update_camera_matrices */

void update_camera(Camera* const camera, const Event* const event, const byte* const heightmap, const byte map_size[2]);
Camera init_camera(const CameraConfig* const config, const GLfloat far_clip_dist);

////////// Sound functions

bool jump_up_sound_activator(const void* const data);
void jump_up_sound_updater(const void* const data, const ALuint al_source);

bool jump_land_sound_activator(const void* const data);
void jump_land_sound_updater(const void* const data, const ALuint al_source);

#endif
