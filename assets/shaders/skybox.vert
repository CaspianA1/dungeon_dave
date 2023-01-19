#version 400 core

#include "common/shared_params.glsl"

layout(location = 0) in ivec3 in_cube_edge;

out vec3 cube_edge;

void main(void) {
	cube_edge = in_cube_edge;
	gl_Position = (mat3x4(view_projection) * cube_edge).xyww;
}
