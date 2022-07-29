#version 400 core

uniform sampler2DArray normal_map_sampler;

vec3 get_tangent_space_normal(const vec3 UV) {
	// Normalized b/c linear filtering may unnormalize it
	return normalize(texture(normal_map_sampler, UV).rgb * 2.0f - 1.0f);
}
