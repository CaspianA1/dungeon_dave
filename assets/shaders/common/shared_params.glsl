#version 400 core

#include "../shadow/num_cascades.glsl"
#include "constants.glsl"

//////////

struct ParallaxMappingParams {
	bool enabled;
	float min_layers, max_layers, height_scale, lod_cutoff;
};

struct ShadowMappingParams {
	uint sample_radius, esm_exponent;

	float
		esm_exponent_layer_scale_factor, inter_cascade_blend_threshold,

		/* TODO: for this and `light_view_projection_matrices`, define a max array size instead,
		and just write less data than that maximum. This will mean that shaders that use those
		arrays will not need to be recompiled per each level. */
		cascade_split_distances[NUM_CASCADE_SPLITS];
};

struct VolumetricLightingParams {
	uint num_samples;
	float sample_density, opacity;
};

struct AmbientOcclusionParams {
	float strength;
};

//////////

/* These are constant for a given level.
They are ordered in terms of the stages of rendering. */
layout(shared) uniform ConstantShadingParams {
	vec2 all_bilinear_percents[NUM_UNIQUE_OBJECT_TYPES];

	ParallaxMappingParams parallax_mapping;
	ShadowMappingParams shadow_mapping;
	VolumetricLightingParams volumetric_lighting;
	AmbientOcclusionParams ambient_occlusion;

	vec3 light_color;
	float tone_mapping_max_white, noise_granularity;
};

layout(shared) uniform DynamicShadingParams {
	vec3 dir_to_light, camera_pos_world_space;
	mat3 billboard_front_facing_tbn; // TODO: infer this from the view matrix
	mat4 view_projection, view, light_view_projection_matrices[NUM_CASCADES];
};
