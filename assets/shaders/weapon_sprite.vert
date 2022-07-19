#version 400 core

#include "common/shadow/shadow.vert"
#include "common/quad_utils.vert"
#include "common/shared_params.glsl"

out vec3 fragment_pos_world_space, UV;

uniform uint frame_index;
uniform vec3 world_corners[CORNERS_PER_QUAD];

void main(void) {
	fragment_pos_world_space = world_corners[gl_VertexID];
	world_depth_value = get_world_depth_value(fragment_pos_world_space);
	UV = vec3(get_quad_UV(), frame_index);
	gl_Position = view_projection * vec4(fragment_pos_world_space, 1.0f);
}
