#version 400 core

#include "common/normal_utils.frag"
#include "common/world_shading.frag"
#include "common/parallax_mapping.frag"

in float world_depth_value;

out vec4 color;

uniform mat3 tbn;

void main(void) {
	vec3 parallax_UV = get_parallax_UV(UV, diffuse_sampler);
	vec3 normal = tbn * get_tangent_space_normal_3D(normal_map_sampler, parallax_UV);

	color = calculate_light(world_depth_value, parallax_UV, normal);
	color.rgb = postprocess_light(parallax_UV.xy, color.rgb);
}
