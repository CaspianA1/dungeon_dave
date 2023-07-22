#version 400 core

#include "common/shared_params.glsl"

layout(location = 0) in ivec3 vertex_pos_world_space;

void main(void) {
	gl_Position = view_projection * vec4(vertex_pos_world_space, 1.0f);
}
