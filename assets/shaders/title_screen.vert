#version 400 core

#include "common/quad_utils.vert"

noperspective out float sliding_UV_x;
noperspective out vec2 UV;
out vec3 pos_difference_from_light;

uniform float palace_city_hori_scroll, palace_city_vert_squish_ratio;
uniform vec3 light_pos_tangent_space;

uniform sampler2D palace_city_diffuse_sampler;

void main(void) {
	UV = get_quad_UV();

	sliding_UV_x = UV.x;
	ivec2 texture_size = textureSize(palace_city_diffuse_sampler, 0);
	sliding_UV_x /= float(texture_size.x) / texture_size.y * palace_city_vert_squish_ratio; // Making the texture aspect ratio correct
	sliding_UV_x += palace_city_hori_scroll;

	gl_Position = vec4(quad_corners[gl_VertexID], 0.0f, 1.0f);
	pos_difference_from_light = vec3(light_pos_tangent_space.xy - gl_Position.xy, light_pos_tangent_space.z);
}
