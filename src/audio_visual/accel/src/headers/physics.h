#ifndef PHYSICS_H
#define PHYSICS_H

#include "utils.h"
#include "camera.h"

// TODO: proper setup of physics constants

#define a_xz 0.15f
#define yv 4.5f
#define xz_v_max 4.0f
#define g 9.81f
#define speed_decel 0.94f

typedef struct {
	GLfloat last_time, view_angle, speed_xz, speed_y;
	vec2 dir_XZ;
	vec3 pos;
} PhysicsEntity;

PhysicsEntity update_physics_entity(PhysicsEntity entity, Event event, byte* const heightmap, const byte map_width, const byte map_height);

#endif
