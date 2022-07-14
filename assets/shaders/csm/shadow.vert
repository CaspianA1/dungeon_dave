#version 400 core

uniform mat4 camera_view;

float get_world_depth_value(const vec3 vertex_pos_world_space) {
	// This is done instead of `-(camera_view * vec4(vertex_pos_world_space, 1.0f)).z`
	vec4 row = vec4(camera_view[0].z, camera_view[1].z, camera_view[2].z, camera_view[3].z);
	return -(dot(row.xyz, vertex_pos_world_space) + row.w);
}
