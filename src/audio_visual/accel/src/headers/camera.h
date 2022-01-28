#ifndef CAMERA_H
#define CAMERA_H

#include "utils.h"
#include "buffer_defs.h"

#define BIT_MOVE_FORWARD 1
#define BIT_MOVE_BACKWARD 2
#define BIT_STRAFE_LEFT 4
#define BIT_STRAFE_RIGHT 8
#define BIT_JUMP 16

//////////

typedef struct {
	struct {
		GLfloat fov, hori, vert, tilt;
	} angles;

	GLfloat pace, time_since_jump; // Pace is the amount of head bob that happens when moving
	Uint64 last_time;

	vec2 right_xz; // X and Z of right (Y is always 0)
	vec3 pos;
	mat4 view_projection, model_view_projection; // Used the least, so last in struct
} Camera;

typedef struct {
	const byte movement_bits; // Tilt right, tilt left, right, left, backward, forward
	int screen_size[2], mouse_movement[2];
} Event;

typedef struct {
	byte* heightmap, map_size[2];
	vec3 speeds;
} PhysicsObject;

/* Excluded: limit_to_pos_neg_domain, update_camera_angles, apply_movement_in_xz_direction,
apply_collision_on_xz_axis, update_pos_via_physics, make_pace_function, smooth_hermite, update_pace */

Event get_next_event(void);
void init_camera(Camera* const camera, const vec3 init_pos);
void update_camera(Camera* const camera, const Event event, PhysicsObject* const physics_obj);

#endif
