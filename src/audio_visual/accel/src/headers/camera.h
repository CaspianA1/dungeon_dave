#ifndef CAMERA_H
#define CAMERA_H

#include "utils.h"
#include "buffer_defs.h"

typedef struct {
	struct {
		GLfloat fov, hori, vert, tilt;
	} angles;

	GLfloat pace, time_since_last_jump; // Pace is the amount of head bob that happens when moving
	Uint64 last_time;

	vec2 right_xz; // X and Z of right (Y is always 0)
	vec3 pos;
	mat4 view_projection, model_view_projection; // Used the least, so last in struct
} Camera;

typedef struct {
	const byte movement_bits; // Tilt right, tilt left, right, left, backward, forward
	int screen_size[2], mouse_dx, mouse_dy; // Delta from last frame
} Event;

typedef struct {
	byte* heightmap, map_size[2];
	vec3 speeds;
} PhysicsObject;

// Excluded: update_camera_angles, apply_movement_in_xz_direction, update_pos_via_physics, make_pace_function, update_pace

Event get_next_event(void);
void init_camera(Camera* const camera, const vec3 init_pos);
void update_camera(Camera* const camera, const Event event, PhysicsObject* const physics_obj);

#endif
