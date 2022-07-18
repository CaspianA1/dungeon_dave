#version 400 core

layout(shared) uniform StaticShadingParams {
	float ambient_strength, diffuse_strength, specular_strength;
	vec2 specular_exponent_domain;

	struct {bool enabled; float max_white;} tone_mapping;

	float noise_granularity;

	vec3 overall_scene_tone, dir_to_light;

	// TODO: only bind a range for these
	vec3 camera_pos_world_space;
	mat4 view_projection;
};
