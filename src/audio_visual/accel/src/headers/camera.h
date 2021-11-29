#ifndef CAMERA_H
#define CAMERA_H

#include "utils.h"

typedef struct {
	vec2 right_xz; // X and Z of right (Y is always 0)
	vec3 pos, dir; // The camera never moves from the origin, but `pos` here is more practical
	GLfloat fov, hori_angle, vert_angle, aspect_ratio;
	mat4 view_projection, model_view_projection; // Used the least, so last in struct
} Camera;

void init_camera(Camera* const camera, const vec3 init_pos);
void update_camera(Camera* const camera);

#endif
