#version 400 core

#include "common/world_shading.frag"
#include "common/normal_utils.frag"

in float world_depth_value;
in vec3 UV;

out vec4 color;

uniform vec2 right_xz;
uniform sampler2DArray diffuse_sampler;

vec3 get_billboard_normal(void) {
	vec3 ts_normal = get_tangent_space_normal(UV); // `ts` = tangent space

	return vec3(
		ts_normal.x * right_xz.x - ts_normal.z * right_xz.y,
		ts_normal.y,
		ts_normal.x * right_xz.y + ts_normal.z * right_xz.x
	);
}

void main(void) {
	vec4 texture_color = texture(diffuse_sampler, UV);
	vec3 light = calculate_light(world_depth_value, texture_color.rgb, get_billboard_normal());
	color = vec4(postprocess_light(UV.xy, light), texture_color.a);
}
