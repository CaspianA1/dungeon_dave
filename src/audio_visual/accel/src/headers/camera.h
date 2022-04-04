#ifndef CAMERA_H
#define CAMERA_H

#include "utils.h"
#include "buffer_defs.h"
#include "event.h"

//////////

typedef struct {
	Uint32 last_time;

	struct {
		GLfloat fov, hori, vert, tilt;
	} angles;

	/* Pace is the amount of head bob that happens when moving.
	The speed xz percent is not the true speed percent; rather,
	the percentage is smoothed out by a Hermite curve. */
	GLfloat pace, speed_xz_percent, time_since_jump, time_accum_for_full_fov;

	vec2 right_xz; // This is used for billboards
	vec3 pos, dir, right, up;

	mat4 model_view_projection;
	vec4 frustum_planes[6];
} Camera;

typedef struct {
	byte* heightmap, map_size[2];
	vec3 velocities;
} PhysicsObject;

/* Excluded: limit_to_pos_neg_domain, get_percent_kept_from,
update_camera_angles, smooth_hermite, update_fov, apply_velocity_in_xz_direction,
tile_exists_at_pos, pos_collides_with_heightmap, update_pos_via_physics,
make_pace_function, update_pace */

void init_camera(Camera* const camera, const vec3 init_pos);
void update_camera(Camera* const camera, const Event event, PhysicsObject* const physics_obj);
void get_dir_in_2D_and_3D(const GLfloat hori_angle, const GLfloat vert_angle, vec2 dir_xz, vec3 dir);

#endif
