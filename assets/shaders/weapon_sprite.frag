#version 400 core

#include "common/world_shading.frag"
#include "common/normal_utils.frag"

in vec3 UV;
in mat3 TBN;

out vec4 color;

void main(void) {
	vec3 normal = normalize(TBN * get_tangent_space_normal(UV));
	float shadow = get_csm_shadow_from_layer(0u, fragment_pos_world_space);

	color = calculate_light_with_provided_shadow(shadow, UV, normal);
	color.rgb = postprocess_light(UV.xy, color.rgb);
}
