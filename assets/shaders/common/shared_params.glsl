#version 400 core

#include "shadow/num_cascades.glsl"

layout(shared) uniform StaticShadingParams {
	struct {float ambient, diffuse, specular;} strengths;

	vec2 specular_exponent_domain;

	struct {bool enabled; float max_white;} tone_mapping;

	float noise_granularity;

	vec3 overall_scene_tone, dir_to_light;

	float cascade_split_distances[NUM_CASCADES - 1u]; // TODO: use the const variable name for this size
};

layout(shared) uniform DynamicShadingParams {
	vec3 camera_pos_world_space;
	mat4 view_projection, view, light_view_projection_matrices[NUM_CASCADES];
};
