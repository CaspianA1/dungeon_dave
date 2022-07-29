#version 400 core

#include "common/shared_params.glsl"

out vec3 UV;

// https://stackoverflow.com/questions/28375338/cube-using-single-gl-triangle-strip
const ivec3 vertices[] = ivec3[](
	ivec3(-1, 1, 1),  ivec3(1, 1, 1),    ivec3(-1, -1, 1), ivec3(1, -1, 1),
	ivec3(1, -1, -1), ivec3(1, 1, 1),    ivec3(1, 1, -1),  ivec3(-1, 1, 1),  ivec3(-1, 1, -1),
	ivec3(-1, -1, 1), ivec3(-1, -1, -1), ivec3(1, -1, -1), ivec3(-1, 1, -1), ivec3(1, 1, -1)
);

void main(void) {
	vec3 vertex_pos_world_space = vertices[gl_VertexID];
	gl_Position = mat3x4(view_projection) * vertex_pos_world_space;

	UV = vertex_pos_world_space;
	UV.x = -UV.x; // Without this, the x component of `UV` is reversed
}
