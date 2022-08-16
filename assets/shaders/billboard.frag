#version 400 core

#include "common/world_shading.frag"
#include "common/normal_utils.frag"

in float world_depth_value;

out vec4 color;

uniform vec2 right_xz;

vec3 get_billboard_normal(void) {
	vec3 ts_normal = get_tangent_space_normal_3D(normal_map_sampler, UV); // `ts` = tangent space

	return vec3(
		ts_normal.x * right_xz.x - ts_normal.z * right_xz.y,
		ts_normal.y,
		ts_normal.x * right_xz.y + ts_normal.z * right_xz.x
	);
}

void main(void) {
	color = calculate_light(world_depth_value, UV, get_billboard_normal());
	color.rgb = postprocess_light(UV.xy, color.rgb);
}
