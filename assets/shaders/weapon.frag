#version 400 core

#include "common/shadow.frag"

noperspective in vec3 fragment_UV;

out vec4 color;

uniform float ambient;
uniform sampler2DArray frame_sampler;

void main(void) {
	color = texture(frame_sampler, fragment_UV);
	color.rgb *= mix(ambient, 1.0f, shadow());

}
