#version 400 core

#include "../shadow/num_cascades.glsl"

/* These are constant for a given level.
They are ordered in terms of the stages of rendering. */
layout(shared) uniform ConstantShadingParams {
	vec2 all_bilinear_percents[3u]; // TODO: make the `3u` an input macro

	struct {
		bool enabled;
		float min_layers, max_layers, height_scale, lod_cutoff;
	} parallax_mapping;

	struct {
		uint sample_radius, esm_exponent;
		float esm_exponent_layer_scale_factor, cascade_split_distances[NUM_CASCADE_SPLITS];
	} shadow_mapping;

	struct {
		bool enabled;
		uint num_samples;
		float sample_density, opacity;
	} volumetric_lighting;

	struct {
		bool tricubic_filtering_enabled;
		float strength;
	} ambient_occlusion;

	vec3 light_color;
	float tone_mapping_max_white, noise_granularity;
};

layout(shared) uniform DynamicShadingParams {
	vec3 dir_to_light, camera_pos_world_space;
	mat3 billboard_front_facing_tbn;
	mat4 view_projection, view, light_view_projection_matrices[NUM_CASCADES];
};
