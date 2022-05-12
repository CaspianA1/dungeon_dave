#version 330 core

in vec3 UV_3D;

out vec3 color;

uniform samplerCube texture_sampler;

void main(void) {
	color = texture(texture_sampler, UV_3D).rgb;
}
