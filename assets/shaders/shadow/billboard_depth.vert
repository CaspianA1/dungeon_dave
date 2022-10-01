#version 400 core

#include "../common/billboard_transform.vert"

uniform vec2 right_xz;

flat out vec3 vertex_UV;

void main(void) {
	vertex_UV = vec3(get_quad_UV(), texture_id);
	vec3 vertex_pos_world_space = get_billboard_vertex(vec3(right_xz.x, 0.0f, right_xz.y));
	gl_Position = vec4(vertex_pos_world_space, 1.0f);
}
