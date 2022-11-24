#version 400 core

#include "../common/quad_utils.vert"

layout(location = 0) in uint billboard_material_index;
layout(location = 1) in uint billboard_texture_id;
layout(location = 2) in vec2 billboard_size_world_space;
layout(location = 3) in vec3 billboard_center_world_space;

vec3 get_billboard_vertex(const vec3 right) {
	vec2 vertex_pos_model_space = quad_corners[gl_VertexID] * 0.5f * billboard_size_world_space;
	vec3 vertex_pos_world_space = vertex_pos_model_space.x * right + billboard_center_world_space;
	vertex_pos_world_space.y += vertex_pos_model_space.y;
	return vertex_pos_world_space;
}
