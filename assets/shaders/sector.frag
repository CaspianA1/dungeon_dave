#version 400 core

#include "common/world_shading.frag"

flat in mat3 tbn;
in float world_depth_value;

out vec3 color;

void main(void) {
	color = calculate_light(world_depth_value, UV, tbn).rgb;
}
