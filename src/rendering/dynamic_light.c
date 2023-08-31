#include "rendering/dynamic_light.h"
#include "data/constants.h" // For `TWO_PI`

DynamicLight init_dynamic_light(const DynamicLightConfig* const config) {
	vec3 from, to;
	glm_vec3_normalize_to((GLfloat*) config -> unnormalized_from, from);
	glm_vec3_normalize_to((GLfloat*) config -> unnormalized_to, to);

	vec3 axis;
	glm_vec3_cross((GLfloat*) from, (GLfloat*) to, axis);
	glm_vec3_normalize(axis);

	return (DynamicLight) {
		config -> time_for_cycle, glm_vec3_angle(from, to),
		{axis[0], axis[1], axis[2]},
		{from[0], from[1], from[2]},
		{from[0], from[1], from[2]} // The initial dir equals `from`
	};
}


void update_dynamic_light(DynamicLight* const dl, const GLfloat curr_time_secs) {
	float (*const movement_function) (const float) = sinf;
	const GLfloat function_period = TWO_PI;

	//////////

	const GLfloat weight = movement_function(curr_time_secs / dl -> time_for_cycle * function_period) * 0.5f + 0.5f;

	versor rotation; // SLERP code based on https://www.gamedev.net/forums/topic/523136-slerping-two-vectors/523136/
	glm_quatv(rotation, dl -> angle_between * weight, (GLfloat*) dl -> axis);
	glm_quat_rotatev(rotation, (GLfloat*) dl -> from, dl -> curr_dir);
}
