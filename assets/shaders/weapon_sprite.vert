#version 400 core

#include "common/quad_utils.vert"
#include "common/world_shading.vert"

uniform uint frame_index;
uniform vec3 world_corners[CORNERS_PER_QUAD];
uniform mat3 tbn;

void main(void) {
	UV = vec3(get_quad_UV(), frame_index);
	set_common_outputs(world_corners[gl_VertexID], tbn);
}
