#version 400 core

#include "common/shadow/shadow.frag"
#include "common/world_shading.frag"

in vec3 UV;

out vec4 color;

uniform vec3 normal;
uniform sampler2DArray diffuse_sampler;

// `quad_with_one_normal` applies to billboards and the weapon sprite

void main(void) {
	vec4 texture_color = texture(diffuse_sampler, UV);

	color = vec4(
		postprocess_light(UV.xy, calculate_light(texture_color.rgb, normal)),
		texture_color.a
	);
}
