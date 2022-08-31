#version 400 core

out vec3 camera_to_fragment_tangent_space;

vec3 get_vector_to_vertex_in_tangent_space(
	const vec3 camera_pos_world_space,
	const vec3 vertex_pos_world_space,
	const mat3 tbn) {

	return tbn * (camera_pos_world_space - vertex_pos_world_space);
}
