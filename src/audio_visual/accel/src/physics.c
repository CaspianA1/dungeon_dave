#ifndef PHYSICS_C
#define PHYSICS_C

#include "headers/physics.h"

// Also take in an event for this
PhysicsEntity update_physics_entity(PhysicsEntity entity) {
	byte moving_forward = 1, jumping = 1, started_jump = 1;

	const GLfloat curr_time = SDL_GetTicks() / 1000.0f;
	const GLfloat delta_time = curr_time - entity.last_time;
	entity.last_time = curr_time;

	const GLfloat delta_speed = a * delta_time;

	// If moving forward, do this
	if (moving_forward) {
		entity.speed[0] += delta_speed * cosf(entity.view_angle);
		entity.speed[2] += delta_speed * sinf(entity.view_angle);
	}
	else {
		entity.speed[0] *= speed_decel;
		entity.speed[2] *= speed_decel;
	}

	if (started_jump) {
		entity.speed[1] = yv;
		started_jump = 0;
		jumping = 1;
	}

	entity.speed[1] += g * delta_time;

	// If under the ground, stop any jump, and clip to ground

	entity.pos[0] += entity.speed[0];
	entity.pos[1] += entity.speed[1];
	entity.pos[2] += entity.speed[2];

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
