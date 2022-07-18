#version 400 core

in vec3 UV;

out vec3 color;

uniform samplerCube skybox_sampler;

void main(void) {
	color = texture(skybox_sampler, UV).rgb;
}
