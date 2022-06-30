#version 400 core

#include "csm/shadow.frag"

in vec3 fragment_pos_world_space, UV;

out vec4 color;

uniform float ambient;
uniform sampler2DArray frame_sampler;

void main(void) {
	const int sample_radius = 5;

	color = texture(frame_sampler, UV); // The weapon layer is always 0, so don't need to call `csm_shadow`
	color.rgb *= mix(ambient, 1.0f, get_csm_shadow_from_layer(0u, fragment_pos_world_space, sample_radius));
}
