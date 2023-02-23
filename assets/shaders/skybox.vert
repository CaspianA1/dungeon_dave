#version 400 core

#include "common/shared_params.glsl"

layout(location = 0) in ivec3 in_cube_edge;

out vec3 cube_edge;

uniform mat3 scale_rotation_matrix;

void main(void) {
	cube_edge = scale_rotation_matrix * in_cube_edge;
	gl_Position = (mat3x4(view_projection) * in_cube_edge).xyww;
}
