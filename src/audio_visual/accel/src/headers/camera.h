#ifndef CAMERA_H
#define CAMERA_H

#include "utils.h"

// TODO: set these constants up properly
#define a_xz 0.2f
#define yv 4.5f
#define xz_v_max 4.0f
#define g 9.81f
#define speed_decel 0.94f

typedef struct {
	struct {
		GLfloat fov, hori, vert, tilt;
	} angles;

	GLfloat last_time;

	vec2 right_xz; // X and Z of right (Y is always 0)
	vec3 pos;
	mat4 view_projection, model_view_projection; // Used the least, so last in struct
} Camera;

typedef struct {
	const byte movement_bits; // Tilt right, tilt left, right, left, backward, forward
	int screen_size[2], mouse_dx, mouse_dy; // Delta from last frame
} Event;

typedef struct {
	GLfloat speed_xz, speed_y;
} PhysicsObject;

// Excluded: update_camera_angles, update_pos_via_physics

Event get_next_event(void);
void init_camera(Camera* const camera, const vec3 init_pos);
void update_camera(Camera* const camera, const Event event, PhysicsObject* const physics_obj);

#endif
