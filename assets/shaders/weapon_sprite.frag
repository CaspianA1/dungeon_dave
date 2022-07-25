#version 400 core

#include "common/world_shading.frag"
#include "common/normal_utils.frag"

in vec3 UV;
in mat3 TBN;

out vec4 color;

uniform sampler2DArray diffuse_sampler;

void main(void) {
	vec4 texture_color = texture(diffuse_sampler, UV);
	vec3 normal = normalize(TBN * get_tangent_space_normal(UV));

	float shadow = get_csm_shadow_from_layer(0u, fragment_pos_world_space);

	color = vec4(
		postprocess_light(UV.xy, calculate_light_with_provided_shadow(texture_color.rgb, normal, shadow)),
		texture_color.a
	);
}
