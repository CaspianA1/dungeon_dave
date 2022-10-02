#version 400 core

#include "common/quad_utils.vert"
#include "common/shared_params.glsl"
#include "common/world_shading.vert"
#include "common/parallax_mapping.vert"

uniform uint frame_index;
uniform vec3 world_corners[CORNERS_PER_QUAD];
uniform mat3 tbn;

void main(void) {
	fragment_pos_world_space = world_corners[gl_VertexID];
	UV = vec3(get_quad_UV(), frame_index);
	ambient_occlusion_UV = get_ambient_occlusion_UV(fragment_pos_world_space);

	camera_to_fragment_tangent_space = get_vector_to_vertex_in_tangent_space(
		camera_pos_world_space, fragment_pos_world_space, tbn
	);

	gl_Position = view_projection * vec4(fragment_pos_world_space, 1.0f);
}
