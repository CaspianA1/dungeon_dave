#version 400 core

#include "csm/shadow.frag"

in vec3 fragment_pos_world_space;
noperspective in vec3 fragment_UV;

out vec4 color;

uniform float ambient;
uniform sampler2DArray frame_sampler;

void main(void) {
	color = texture(frame_sampler, fragment_UV);
	color.rgb *= mix(ambient, 1.0f, in_csm_shadow(fragment_pos_world_space));
}
