#version 400 core

#include "common/world_shading.frag"

out vec3 color;

void main(void) {
	color = calculate_light().rgb;
}
