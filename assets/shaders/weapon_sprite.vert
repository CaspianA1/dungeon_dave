#version 400 core

#include "common/shadow/shadow.vert"
#include "common/quad_utils.vert"
#include "common/shared_params.glsl"

out vec3 fragment_pos_world_space, UV;
out mat3 TBN;

uniform uint frame_index;
uniform vec3 face_normal, face_tangent, world_corners[CORNERS_PER_QUAD];

void main(void) {
	fragment_pos_world_space = world_corners[gl_VertexID];
	world_depth_value = get_world_depth_value(view, fragment_pos_world_space);

	UV = vec3(get_quad_UV(), frame_index);
	TBN = mat3(face_tangent, cross(face_tangent, face_normal), face_normal);
	gl_Position = view_projection * vec4(fragment_pos_world_space, 1.0f);
}
