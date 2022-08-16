#version 400 core

#include "common/world_shading.frag"
#include "common/normal_utils.frag"

out vec4 color;

uniform mat3 tbn;

void main(void) {
	// No need to renormalize here, since the TBN matrix's components are already normalized
	vec3 normal = tbn * get_tangent_space_normal_3D(normal_map_sampler, UV);
	float shadow_strength = get_csm_shadow_from_layer(0u, fragment_pos_world_space);

	color = calculate_light_with_provided_shadow_strength(shadow_strength, UV, normal);
	color.rgb = postprocess_light(UV.xy, color.rgb);
}
