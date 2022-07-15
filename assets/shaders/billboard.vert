#version 400 core

#include "csm/shadow.vert"
#include "quad.vert"

layout(location = 0) in uint texture_id;
layout(location = 1) in vec2 billboard_size_world_space;
layout(location = 2) in vec3 billboard_center_world_space;

out float world_depth_value;
out vec3 fragment_pos_world_space, UV;

uniform vec2 right_xz_world_space;
uniform mat4 view_projection;

void main(void) {
	vec2 vertex_pos_model_space = quad_corners[gl_VertexID] * 0.5f * billboard_size_world_space;

	fragment_pos_world_space = billboard_center_world_space
		+ vertex_pos_model_space.x * vec3(right_xz_world_space, 0.0f).xzy
		+ vec3(0.0f, vertex_pos_model_space.y, 0.0f);

	world_depth_value = get_world_depth_value(fragment_pos_world_space);
	UV = vec3(get_quad_UV(), texture_id);
	gl_Position = view_projection * vec4(fragment_pos_world_space, 1.0f);
}
