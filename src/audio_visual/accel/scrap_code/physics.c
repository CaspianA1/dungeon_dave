#ifndef PHYSICS_C
#define PHYSICS_C

#include "headers/physics.h"

// TODO: make a clearer distinction between physics entities and the camera

void update_newtonian_camera(
	GLfloat* const speed_xz_ref, GLfloat* const speed_y_ref,
	Camera* const camera, const Event event,
	byte* const heightmap, const byte map_width, const byte map_height) {

	GLfloat speed_xz = *speed_xz_ref, speed_y = *speed_y_ref;
	vec3 pos;
	memcpy(pos, camera -> pos, sizeof(vec3));

	/* - Strafing
	- Clipping before hitting walls head-on
	- Tilt when turning (make a fn rotate_camera_as_needed for that)
	- Make speed when pushing against wall depend on surface normal
	- Integrate physics system with camera system */

	const byte moving_forward = event.movement_bits & 1, moving_backward = event.movement_bits & 2;

	const GLfloat curr_time = SDL_GetTicks() / 1000.0f;
	const GLfloat delta_time = curr_time - camera -> last_time;
	camera -> last_time = curr_time;

	const GLfloat delta_speed = a_xz * delta_time, max_speed_xz = xz_v_max * delta_time;

	if (moving_forward && ((speed_xz += delta_speed) > max_speed_xz)) speed_xz = max_speed_xz;
	if (moving_backward && ((speed_xz -= delta_speed) < -max_speed_xz)) speed_xz = -max_speed_xz;

	if (!moving_forward && !moving_backward) speed_xz *= speed_decel;

	////////// X and Z collision detection + setting new positions
	camera -> dir_xz[0] = sinf(camera -> hori_angle);
	camera -> dir_xz[1] = cosf(camera -> hori_angle);

	const GLfloat new_x = pos[0] + speed_xz * camera -> dir_xz[0];
	byte height;

	if (new_x >= 0.0f && new_x <= map_width - 1.0f) {
		height = *map_point(heightmap, new_x, pos[2], map_width);
		if (height <= pos[1]) pos[0] = new_x;
		// else entity.speed_xz /= 2.0f;
	}

	const GLfloat new_z = pos[2] + speed_xz * camera -> dir_xz[1];
	if (new_z >= 0.0f && new_z <= map_height - 1.0f) {
		height = *map_point(heightmap, pos[0], new_z, map_width);
		if (height <= pos[1]) pos[2] = new_z;
		// else entity.speed_xz /= 2.0f;
	}

	////////// Y collision detection + setting new positions

	if (speed_y == 0.0f && keys[SDL_SCANCODE_SPACE]) speed_y = yv;
	else speed_y -= g * delta_time;

	const GLfloat new_y = pos[1] + speed_y * delta_time; // Since g works w acceleration
	height = *map_point(heightmap, pos[0], pos[2], map_width);

	if (new_y > height) pos[1] = new_y;
	else {
		speed_y = 0.0f;
		pos[1] = height;
	}

	*speed_xz_ref = speed_xz;
	*speed_y_ref = speed_y;
	memcpy(camera -> pos, pos, sizeof(vec3));
}

/*
a = x and z acceleration
g = gravity-based downward acceleration
dt = time passed for last tick
v = x and z velocity
yv = y jump velocity
p = position

dv = a * dt
v.x += dv * dir.x
v.z += dv * dir.z

Jumping:
Init: if not jumping, v.y = yv
v.y += g * t
If touching the ground, v.y = 0

p += v (is that right?)
*/

#endif
