#ifndef PHYSICS_C
#define PHYSICS_C

#include "headers/physics.h"

// TODO: make a clearer distinction between physics entities and the camera

PhysicsEntity update_physics_entity(PhysicsEntity entity, Event event, byte* const heightmap, const byte map_width, const byte map_height) {
	// TODO: strafing and clipping before hitting walls head-on
	// Tilt when turning (make a fn rotate_camera_as_needed for that)

	const byte moving_forward = event.movement_bits & 1, moving_backward = event.movement_bits & 2;

	const GLfloat curr_time = SDL_GetTicks() / 1000.0f;
	const GLfloat delta_time = curr_time - entity.last_time;
	entity.last_time = curr_time;

	const GLfloat delta_speed = a_xz * delta_time, max_speed_xz = xz_v_max * delta_time;

	if (moving_forward && ((entity.speed_xz += delta_speed) > max_speed_xz))
		entity.speed_xz = max_speed_xz;

	if (moving_backward && ((entity.speed_xz -= delta_speed) < -max_speed_xz))
		entity.speed_xz = -max_speed_xz;

	if (!moving_forward && !moving_backward) entity.speed_xz *= speed_decel;

	////////// X and Z collision detection + setting new positions
	entity.dir_XZ[0] = sinf(entity.view_angle);
	entity.dir_XZ[1] = cosf(entity.view_angle);

	const GLfloat new_x = entity.pos[0] + entity.speed_xz * entity.dir_XZ[0];
	byte height;

	if (new_x >= 0.0f && new_x <= map_width - 1.0f) {
		height = *map_point(heightmap, new_x, entity.pos[2], map_width);
		if (height <= entity.pos[1]) entity.pos[0] = new_x;
	}

	const GLfloat new_z = entity.pos[2] + entity.speed_xz * entity.dir_XZ[1];
	if (new_z >= 0.0f && new_z <= map_height - 1.0f) {
		height = *map_point(heightmap, entity.pos[0], new_z, map_width);
		if (height <= entity.pos[1]) entity.pos[2] = new_z;
	}

	////////// Y collision detection + setting new positions

	if (entity.speed_y == 0.0f && keys[SDL_SCANCODE_SPACE])
		entity.speed_y = yv;
	else entity.speed_y -= g * delta_time;

	const GLfloat new_y = entity.pos[1] + entity.speed_y * delta_time; // Since g works w acceleration
	height = *map_point(heightmap, entity.pos[0], entity.pos[2], map_width);

	if (new_y > height) entity.pos[1] = new_y;
	else {
		entity.speed_y = 0.0f;
		entity.pos[1] = height;
	}

	return entity;
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
