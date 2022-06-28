#version 400 core

#include "csm/shadow.frag"

in float world_depth_value;
in vec3 UV, fragment_pos_world_space;
out vec4 color;

uniform float ambient;
uniform sampler2DArray texture_sampler;

void main(void) {
	color = texture(texture_sampler, UV);
	color.rgb *= mix(ambient, 1.0f, csm_shadow(world_depth_value, fragment_pos_world_space));

	float divisor = mix(color.a, 1.0f, color.a == 0.0f); // Avoiding division by zero
	color.rgb /= divisor;
}
