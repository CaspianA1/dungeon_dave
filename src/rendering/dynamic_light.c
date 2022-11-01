#include "rendering/dynamic_light.h"
#include "data/constants.h"

DynamicLight init_dynamic_light(const GLfloat time_for_cycle,
	const vec3 pos, const vec3 looking_at_1, const vec3 looking_at_2) {

	vec3 dir_1, dir_2;

	glm_vec3_sub((GLfloat*) pos, (GLfloat*) looking_at_1, dir_1);
	glm_vec3_normalize(dir_1);

	glm_vec3_sub((GLfloat*) pos, (GLfloat*) looking_at_2, dir_2);
	glm_vec3_normalize(dir_2);

	return (DynamicLight) {
		time_for_cycle,
		{dir_1[0], dir_1[1], dir_1[2]},
		{dir_2[0], dir_2[1], dir_2[2]},
		{dir_2[0], dir_2[1], dir_2[2]} // The initial dir equals `dir_1`
	};
}

void update_dynamic_light(DynamicLight* const dl, const GLfloat curr_time_secs) {
	const GLfloat weight = sinf(curr_time_secs / dl -> time_for_cycle * TWO_PI) * 0.5f + 0.5f;

	GLfloat* const curr_dir = dl -> curr_dir;
	glm_vec3_lerp((GLfloat*) dl -> dir_2, (GLfloat*) dl -> dir_1, weight, curr_dir);
	glm_vec3_normalize(curr_dir);
}
