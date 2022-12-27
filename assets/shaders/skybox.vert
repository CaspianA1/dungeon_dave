#version 400 core

#include "common/shared_params.glsl"

out vec3 cube_edge;

// https://stackoverflow.com/questions/28375338/cube-using-single-gl-triangle-strip
const ivec3 vertices[] = ivec3[](
	ivec3(-1, 1, 1),  ivec3(1, 1, 1),    ivec3(-1, -1, 1), ivec3(1, -1, 1),
	ivec3(1, -1, -1), ivec3(1, 1, 1),    ivec3(1, 1, -1),  ivec3(-1, 1, 1),  ivec3(-1, 1, -1),
	ivec3(-1, -1, 1), ivec3(-1, -1, -1), ivec3(1, -1, -1), ivec3(-1, 1, -1), ivec3(1, 1, -1)
);

void main(void) {
	cube_edge = vertices[gl_VertexID];
	gl_Position = (mat3x4(view_projection) * cube_edge).xyww;
}
