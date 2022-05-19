#version 330 core

#include "common/shadow.frag"

in vec3 UV;
out vec4 color;

uniform float ambient;
uniform sampler2DArray texture_sampler;

void main(void) {
	color = texture(texture_sampler, UV);
	color.rgb *= mix(ambient, 1.0f, shadow());
}
