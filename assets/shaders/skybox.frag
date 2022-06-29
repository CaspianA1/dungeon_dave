#version 400 core

in vec3 UV;

out vec3 color;

uniform samplerCube texture_sampler;

void main(void) {
	color = texture(texture_sampler, UV).rgb;
}
