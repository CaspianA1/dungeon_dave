#version 400 core

#include "shadow/shadow.vert"
#include "common/shared_params.glsl"
#include "common/quad_utils.vert"
#include "common/world_shading.vert"
#include "common/parallax_mapping.vert"

layout(location = 0) in uint texture_id;
layout(location = 1) in vec2 billboard_size_world_space;
layout(location = 2) in vec3 billboard_center_world_space;

uniform mat3 tbn;

void main(void) {
	vec3 right = tbn[0];
	right.z = -right.z;

	vec2 vertex_pos_model_space = quad_corners[gl_VertexID] * 0.5f * billboard_size_world_space;
	fragment_pos_world_space = vertex_pos_model_space.x * right + billboard_center_world_space;
	fragment_pos_world_space.y += vertex_pos_model_space.y;

	world_depth_value = get_world_depth_value(view, fragment_pos_world_space);

	UV = vec3(get_quad_UV(), texture_id);
	ambient_occlusion_UV = get_ambient_occlusion_UV(fragment_pos_world_space);

	camera_to_fragment_tangent_space = get_vector_to_vertex_in_tangent_space(
		camera_pos_world_space, fragment_pos_world_space, tbn
	);

	gl_Position = view_projection * vec4(fragment_pos_world_space, 1.0f);
}
