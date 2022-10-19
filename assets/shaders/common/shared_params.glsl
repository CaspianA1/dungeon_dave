#version 400 core

#include "../shadow/num_cascades.glsl"

/* These are constant for a given level.
They are ordered in terms of the stages of rendering. */
layout(shared) uniform ConstantShadingParams {
	struct {
		bool enabled;
		float min_layers, max_layers, height_scale, lod_cutoff;
	} parallax_mapping;

	struct {float diffuse, normal;} bilinear_percents;
	struct {float ambient, diffuse, specular;} strengths;
	struct {float matte, rough;} specular_exponents;

	struct {
		uint sample_radius, esm_exponent;
		float esm_exponent_layer_scale_factor, cascade_split_distances[NUM_CASCADE_SPLITS];
	} shadow_mapping;

	vec3 overall_scene_tone;
	float tone_mapping_max_white, noise_granularity;
};

layout(shared) uniform DynamicShadingParams {
	vec3 dir_to_light, camera_pos_world_space;
	mat4 view_projection, view, light_view_projection_matrices[NUM_CASCADES];
};
