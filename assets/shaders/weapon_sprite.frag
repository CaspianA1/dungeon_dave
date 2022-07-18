#version 400 core

#include "shadow/shadow.frag"

in vec3 fragment_pos_world_space, UV;

out vec4 color;

uniform float ambient_strength;
uniform sampler2DArray frame_sampler;

void main(void) {
	color = texture(frame_sampler, UV); // The weapon layer is always 0, so no need to call `get_csm_shadow`
	color.rgb *= mix(ambient_strength, 1.0f, get_csm_shadow_from_layer(0u, fragment_pos_world_space));
}
