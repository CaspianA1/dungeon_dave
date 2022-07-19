#version 400 core

in vec3 UV;

out vec4 color;

uniform vec3 normal;
uniform sampler2DArray diffuse_sampler;

#include "shadow/shadow.frag"
#include "world_shading.frag"

void main(void) {
	vec4 texture_color = texture(diffuse_sampler, UV);

	color = vec4(
		postprocess_light(UV.xy, calculate_light(texture_color.rgb, normal)),
		texture_color.a
	);
}
