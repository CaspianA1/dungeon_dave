#version 400 core

#include "../quad_utils.vert"

layout(location = 0) in vec3 vertex_pos_world_space;

out vec2 translucent_vertex_quad_UV;

void main(void) {
	gl_Position = vec4(vertex_pos_world_space, 1.0f);
	translucent_vertex_quad_UV = get_quad_UV();
}
