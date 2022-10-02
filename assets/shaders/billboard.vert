#version 400 core

#include "shadow/shadow.vert"
#include "common/shared_params.glsl"
#include "common/world_shading.vert"
#include "common/parallax_mapping.vert"
#include "common/billboard_transform.vert"

uniform mat3 tbn;

// TODO: billboards are not correctly lit when looking at them from behind
void main(void) {
	fragment_pos_world_space = get_billboard_vertex(-tbn[0]);

	world_depth_value = get_world_depth_value(view, fragment_pos_world_space);

	UV = vec3(get_quad_UV(), texture_id);
	ambient_occlusion_UV = get_ambient_occlusion_UV(fragment_pos_world_space);

	camera_to_fragment_tangent_space = get_vector_to_vertex_in_tangent_space(
		camera_pos_world_space, fragment_pos_world_space, tbn
	);

	gl_Position = view_projection * vec4(fragment_pos_world_space, 1.0f);
}
