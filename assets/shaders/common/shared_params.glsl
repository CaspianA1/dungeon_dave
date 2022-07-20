#version 400 core

layout(shared) uniform StaticShadingParams {
	struct {float ambient, diffuse, specular;} strengths;

	vec2 specular_exponent_domain;

	struct {bool enabled; float max_white;} tone_mapping;

	float noise_granularity;

	vec3 overall_scene_tone, dir_to_light;
};

layout(shared) uniform DynamicShadingParams {
	vec3 camera_pos_world_space;
	mat4 view_projection, view;
};
