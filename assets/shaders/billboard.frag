#version 400 core

#include "common/world_shading.frag"

in float world_depth_value;

out vec4 color;

flat in mat3 tbn;

void main(void) {
	color = calculate_light(world_depth_value, UV, tbn);
}
