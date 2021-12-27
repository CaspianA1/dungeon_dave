#ifndef PHYSICS_H
#define PHYSICS_H

/* TODO: physics constants:
flat_a (player x and z acceleration)
g (gravity downward acceleration)
up_a (player y jump acceleration)
v_max (max speed in any direction) */

#define a 3.2f
#define yv 2.0f
#define g 9.8f
#define speed_decel 0.95f

typedef struct {
	GLfloat last_time, view_angle;
	vec3 pos, speed;
} PhysicsEntity;

PhysicsEntity update_physics_entity(PhysicsEntity entity);

#endif
