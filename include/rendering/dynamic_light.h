#ifndef DYNAMIC_LIGHT_H
#define DYNAMIC_LIGHT_H

#include "lib/glad/glad.h" // For OpenGL defs
#include "utils/cglm_include.h" // For `vec3`

typedef struct {
	const GLfloat time_for_cycle;
	const vec3 pos;
	const struct {const vec3 origin, dest;} looking_at;
} DynamicLightConfig;

// This light is dynamic in the sense that it cycles between 2 directions.
typedef struct {
	const GLfloat time_for_cycle, angle_between;
	const vec3 axis, from;
	vec3 curr_dir;
} DynamicLight;

//////////

DynamicLight init_dynamic_light(const DynamicLightConfig* const config);
void update_dynamic_light(DynamicLight* const dl, const GLfloat curr_time_secs);

#endif
