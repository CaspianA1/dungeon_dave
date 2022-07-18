#version 400 core

#include "shadow/shadow.vert"
#include "quad.vert"

out vec3 fragment_pos_world_space, UV;

uniform uint frame_index;
uniform vec3 world_corners[4];
uniform mat4 view_projection;

void main(void) {
	fragment_pos_world_space = world_corners[gl_VertexID];
	UV = vec3(get_quad_UV(), frame_index);
	gl_Position = view_projection * vec4(fragment_pos_world_space, 1.0f);
}
