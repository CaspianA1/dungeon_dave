#version 400 core

#include "csm/shadow.frag"

in vec3 fragment_pos_world_space;
noperspective in vec3 fragment_UV;

out vec4 color;

uniform float ambient;
uniform sampler2DArray frame_sampler;

void main(void) {
	color = texture(frame_sampler, fragment_UV);

	// Layer is always 0 for weapon, so `in_csm_shadow` doesn't need to be called
	color.rgb *= mix(ambient, 1.0f, get_csm_shadow_from_layer(0u, fragment_pos_world_space));
}
