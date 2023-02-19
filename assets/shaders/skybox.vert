#version 400 core

#include "common/shared_params.glsl"

layout(location = 0) in ivec3 in_cube_edge;

out vec3 cube_edge;

uniform float horizon_dist_scale;

void main(void) {
	cube_edge = in_cube_edge;
	cube_edge.y *= horizon_dist_scale;
	gl_Position = (mat3x4(view_projection) * in_cube_edge).xyww;
}
