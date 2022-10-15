#ifndef DYNAMIC_LIGHT_H
#define DYNAMIC_LIGHT_H

#include "utils/buffer_defs.h"

// This light is dynamic in the sense that it can cycle between 2 directions.
typedef struct {
	const GLfloat time_for_cycle;
	const vec3 dir_1, dir_2;
	vec3 curr_dir;
} DynamicLight;

//////////

DynamicLight init_dynamic_light(const GLfloat time_for_cycle,
	const vec3 pos, const vec3 looking_at_1, const vec3 looking_at_2);

void update_dynamic_light(DynamicLight* const dl, const GLfloat curr_time_secs);

#endif
