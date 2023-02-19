#version 400 core

#include "common/shared_params.glsl"

in vec3 cube_edge;

out vec3 color;

uniform bool apply_cylindrical_projection;
uniform float y_shift_offset;

uniform samplerCube skybox_sampler;

void main(void) {
	vec3 remapped_edge = cube_edge;

	// https://stackoverflow.com/questions/75466490
	if (apply_cylindrical_projection) {
		remapped_edge /= length(remapped_edge.xz);
		remapped_edge.xz /= max(abs(remapped_edge.x), abs(remapped_edge.z));

		const float one_over_atan_of_one = 1.0f / atan(1.0f);
		remapped_edge.xz = atan(remapped_edge.xz) * one_over_atan_of_one;
	}

	remapped_edge.y += y_shift_offset;
	color = texture(skybox_sampler, remapped_edge).rgb * light_color;
}
