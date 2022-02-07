#ifndef CAMERA_H
#define CAMERA_H

#include "utils.h"
#include "buffer_defs.h"

#define BIT_MOVE_FORWARD 1
#define BIT_MOVE_BACKWARD 2
#define BIT_STRAFE_LEFT 4
#define BIT_STRAFE_RIGHT 8
#define BIT_JUMP 16
#define BIT_ACCELERATE 32

//////////

typedef struct {
	Uint64 last_time;

	struct {
		GLfloat fov, hori, vert, tilt;
	} angles;

	/* Pace is the amount of head bob that happens when moving.
	The speed xz percent is not the true speed percent; rather,
	the percentage is smoothed out by a Hermite curve. */
	GLfloat pace, time_since_jump, time_accum_for_full_fov;

	vec2 right_xz; // X and Z of right (Y is always 0)
	vec3 pos;

	mat4 model_view_projection; // Used the least, so last in struct
	vec4 frustum_planes[6];

} Camera;

typedef struct {
	const byte movement_bits; // Tilt right, tilt left, right, left, backward, forward
	int screen_size[2], mouse_movement[2];
} Event;

typedef struct {
	byte* heightmap, map_size[2];
	vec3 velocities;
} PhysicsObject;

/* Excluded: limit_to_pos_neg_domain, update_camera_angles,
smooth_hermite, update_fov, apply_velocity_in_xz_direction,
tile_exists_at_pos, pos_collides_with_heightmap, update_pos_via_physics,
make_pace_function, update_pace, get_view_matrix */

Event get_next_event(void);
void init_camera(Camera* const camera, const vec3 init_pos);
void update_camera(Camera* const camera, const Event event, PhysicsObject* const physics_obj);

#endif
