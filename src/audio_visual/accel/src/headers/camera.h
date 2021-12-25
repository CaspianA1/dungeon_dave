#ifndef CAMERA_H
#define CAMERA_H

#include "utils.h"

typedef struct {
	GLfloat fov, hori_angle, vert_angle, tilt_angle, last_time; // Can fit 6 more GLfloats without padding overflow
	vec2 right_xz; // X and Z of right (Y is always 0)
	vec3 pos;
	mat4 view_projection, model_view_projection; // Used the least, so last in struct
} Camera;

typedef struct {
	const byte movement_bits; // Tilt right, tilt left, right, left, backward, forward
	int screen_size[2], mouse_dx, mouse_dy; // Delta from last frame
} Event;

// Excluded: update_camera

Event get_next_event(void);
void init_camera(Camera* const camera, const vec3 init_pos);
void update_camera_from_event(Camera* const camera, const Event event);

#endif
