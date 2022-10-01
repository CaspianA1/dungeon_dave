#version 400 core

out float world_depth_value;

float get_world_depth_value(const mat4 view, const vec3 vertex_pos_world_space) {
	// This is done instead of `-(view * vec4(vertex_pos_world_space, 1.0f)).z`
	vec4 row = vec4(view[0].z, view[1].z, view[2].z, view[3].z);
	return -(dot(row.xyz, vertex_pos_world_space) + row.w);
}
