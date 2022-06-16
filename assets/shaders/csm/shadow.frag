#version 400 core

#include "num_cascades.geom" // `num_cascades.geom` is written to by the CPU before any shader compilation

const uint NUM_CASCADE_SPLITS = NUM_CASCADES - 1u;

// TODO: share `light_view_projection_matrices` with `depth.geom`

uniform float cascade_split_distances[NUM_CASCADE_SPLITS];
uniform mat4 light_view_projection_matrices[NUM_CASCADES];
uniform sampler2DArray shadow_cascade_sampler;

float get_average_occluder_depth(vec2 UV, uint layer_index, int sample_radius) {
	float average_occluder_depth = 0.0f;
	vec2 texel_size = 1.0f / textureSize(shadow_cascade_sampler, 0).xy;

	for (int y = -sample_radius; y <= sample_radius; y++) {
		for (int x = -sample_radius; x <= sample_radius; x++) {
			vec3 sample_UV = vec3(texel_size * vec2(x, y) + UV.xy, layer_index);
			average_occluder_depth += texture(shadow_cascade_sampler, sample_UV).r;
		}
	}

	int samples_across = (sample_radius << 1) + 1;
	return average_occluder_depth / (samples_across * samples_across);
}

float get_csm_shadow_from_layer(uint layer_index, float percent_between_cascades, vec3 fragment_pos_world_space) {
	const int sample_radius = 1;
	const float esm_constant = 300.0f, layer_scaling_component = 1.5f; // Terrain: 1.2f. Palace: 1.5f.

	/* (TODO) esm scaling:
	- Bigger depth range will be darker, so scale the exponent primarily on that
	- Secondarily, depth values will be different b/c depth values are normalized
		based on the cascade depth range, so also rescale also on the cascade */

	/////////// Getting UV

	vec4 fragment_pos_light_space = light_view_projection_matrices[layer_index] * vec4(fragment_pos_world_space, 1.0f);
	vec3 UV = fragment_pos_light_space.xyz * 0.5f + 0.5f;

	/////////// Calculating the shadow strength

	float fractional_layer_index = layer_index + percent_between_cascades;
	float layer_scaled_esm_constant = esm_constant * pow(fractional_layer_index + 1u, layer_scaling_component);

	float occluder_receiver_diff = UV.z - get_average_occluder_depth(UV.xy, layer_index, sample_radius);
	float in_light_percentage = exp(-layer_scaled_esm_constant * occluder_receiver_diff);
	return clamp(in_light_percentage, 0.0f, 1.0f);
}

float get_blended_csm_shadow(uint layer_index, uint depth_range_shift, float world_depth_value, vec3 fragment_pos_world_space) {
	// If the layer index equals 0, this makes the previous layer 0
	uint prev_layer_index = max(int(layer_index) - 1, 0);

	float
		dist_ahead_of_last_split = world_depth_value - cascade_split_distances[prev_layer_index],
		depth_range = cascade_split_distances[layer_index - depth_range_shift] -
					  cascade_split_distances[prev_layer_index - depth_range_shift];

	/* If the layer index equals 0, this will be less than 0; and if it's the last layer index,
	it may be over 1. This clamps the blend factor between 0 and 1 for when that happens. */
	float percent_between = clamp(dist_ahead_of_last_split / depth_range, 0.0f, 1.0f);

	return mix(
		get_csm_shadow_from_layer(prev_layer_index, percent_between, fragment_pos_world_space),
		get_csm_shadow_from_layer(layer_index, percent_between, fragment_pos_world_space),
		percent_between
	);
}

float csm_shadow(float world_depth_value, vec3 fragment_pos_world_space) {
	uint layer_index = 0;

	while (layer_index < NUM_CASCADE_SPLITS
		&& cascade_split_distances[layer_index] <= world_depth_value)
		layer_index++;

	bool on_last_split = layer_index == NUM_CASCADE_SPLITS;
	return get_blended_csm_shadow(layer_index, int(on_last_split), world_depth_value, fragment_pos_world_space);
}
