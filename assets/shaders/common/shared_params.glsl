#version 400 core

#include "shadow/num_cascades.glsl"

/* These are constant for a given level.
They are ordered in terms of the stages of rendering. */
layout(shared) uniform ConstantShadingParams {
	vec3 dir_to_light;

	struct {float bilinear, ao;} percents;
	struct {float ambient, diffuse, specular;} strengths;
	struct {float matte, rough;} specular_exponents;

	float
		specular_exponent,
		cascade_split_distances[NUM_CASCADES - 1u], // TODO: use the const variable name for this size
		tone_mapping_max_white, noise_granularity;

	vec3 overall_scene_tone;
};

layout(shared) uniform DynamicShadingParams {
	vec3 camera_pos_world_space;
	mat4 view_projection, view, light_view_projection_matrices[NUM_CASCADES];
};
