#version 400 core

#include "common/world_shading.frag"

out vec4 color;

uniform mat3 tbn;

void main(void) {
	float shadow_strength = get_csm_shadow_from_layer(0u, fragment_pos_world_space);
	color = calculate_light_with_provided_shadow_strength(shadow_strength, UV, tbn);
}
