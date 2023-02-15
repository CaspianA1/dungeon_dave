#version 400 core

#include "common/shared_params.glsl"

in vec3 cube_edge;

out vec3 color;

uniform samplerCube skybox_sampler;

void main(void) {
	color = texture(skybox_sampler, cube_edge).rgb * light_color;
}
