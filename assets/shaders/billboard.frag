#version 400 core

#include "csm/shadow.frag"

in float world_depth_value;
in vec3 fragment_pos_world_space, UV;
out vec4 color;

uniform float ambient;
uniform sampler2DArray texture_sampler;

void main(void) {
	color = texture(texture_sampler, UV);
	color.rgb *= mix(ambient, 1.0f, get_csm_shadow(world_depth_value, fragment_pos_world_space));
	color.rgb /= mix(color.a, 1.0f, color.a == 0.0f); // Avoiding division by zero
}
