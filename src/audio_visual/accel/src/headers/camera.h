#ifndef CAMERA_H
#define CAMERA_H

#include "utils.h"

// TODO: set these constants up properly
#define a_forward_back 0.38f
#define a_strafe 0.4f
#define yv 5.5f
#define xz_v_max 4.0f
#define g 13.0f
#define speed_decel 0.87f

typedef struct {
	struct {
		GLfloat fov, hori, vert, tilt;
	} angles;

	// `bob` is the bouncing up and down of camera when moving
	GLfloat last_time, bob_input, last_bob_delta;

	vec2 right_xz; // X and Z of right (Y is always 0)
	vec3 pos;
	mat4 view_projection, model_view_projection; // Used the least, so last in struct
} Camera;

typedef struct {
	const byte movement_bits; // Tilt right, tilt left, right, left, backward, forward
	int screen_size[2], mouse_dx, mouse_dy; // Delta from last frame
} Event;

typedef struct {
	byte* const heightmap;
	const byte map_size[2];
	vec3 speeds;
} PhysicsObject;

// Excluded: update_camera_angles, update_pos_via_physics, update_bob

Event get_next_event(void);
void init_camera(Camera* const camera, const vec3 init_pos);
void update_camera(Camera* const camera, const Event event, PhysicsObject* const physics_obj);

#endif
