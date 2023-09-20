#version 400 core

#include "common/quad_utils.vert"
#include "common/shared_params.glsl"

out vec3 UV;

uniform uint frame_index;
uniform vec3 world_corners[CORNERS_PER_QUAD];

void main(void) {
	UV = vec3(get_quad_UV(), frame_index);
	gl_Position = view_projection * vec4(world_corners[gl_VertexID], 1.0f);
}
