#version 400 core

#include "common/quad_utils.vert"

noperspective out vec2 UV;
out vec3 pos_difference_from_light;

uniform vec3 light_pos_tangent_space;

void main(void) {
	UV = get_quad_UV();
	vec2 quad_corner = quad_corners[gl_VertexID];
	pos_difference_from_light = vec3(vec2(light_pos_tangent_space.xy - quad_corner), light_pos_tangent_space.z);
	gl_Position = vec4(quad_corner, 0.0f, 1.0f);
}
