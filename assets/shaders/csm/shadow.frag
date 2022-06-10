#version 400 core

#include "num_cascades.geom" // `num_cascades.geom` is written to by the CPU before any shader compilation

// TODO: share `light_view_projection_matrices` with `depth.geom`

uniform float cascade_plane_distances[NUM_CASCADES - 1];
uniform mat4 light_view_projection_matrices[NUM_CASCADES];
uniform sampler2DArray shadow_cascade_sampler;

float get_average_occluder_depth(vec2 UV, uint layer, int sample_radius) {
	float average_occluder_depth = 0.0f;
	vec2 texel_size = 1.0f / textureSize(shadow_cascade_sampler, 0).xy;

	for (int y = -sample_radius; y <= sample_radius; y++) {
		for (int x = -sample_radius; x <= sample_radius; x++) {
			vec3 sample_UV = vec3(texel_size * vec2(x, y) + UV.xy, layer);
			average_occluder_depth += texture(shadow_cascade_sampler, sample_UV).r;
		}
	}

	int samples_across = (sample_radius << 1) + 1;
	return average_occluder_depth / (samples_across * samples_across);
}

float get_csm_shadow_from_layer(uint layer, vec3 fragment_pos_world_space) {
	const int sample_radius = 0;
	const float esm_constant = 300.0f, layer_scaling_component = 1.2f;

	/* (TODO) esm scaling:
	- Bigger depth range will be darker, so scale the exponent primarily on that
	- Secondarily, depth values will be different b/c depth values are normalized
		based on the cascade depth range, so also rescale also on the cascade */

	/////////// Getting UV

	vec4 fragment_pos_light_space = light_view_projection_matrices[layer] * vec4(fragment_pos_world_space, 1.0f);
	vec3 UV = fragment_pos_light_space.xyz * 0.5f + 0.5f;

	/////////// Calculating the shadow strength

	float occluder_receiver_diff = UV.z - get_average_occluder_depth(UV.xy, layer, sample_radius);
	float layer_scaled_esm_constant = esm_constant * pow(layer + 1u, layer_scaling_component);
	float in_light_percentage = exp(-layer_scaled_esm_constant * occluder_receiver_diff);
	return clamp(in_light_percentage, 0.0f, 1.0f);
}

float csm_shadow(float world_depth_value, vec3 fragment_pos_world_space) {
	const int num_splits_between_cascades = int(NUM_CASCADES) - 1;
	int layer = num_splits_between_cascades; // TODO: select the cascade using some constant-time math

	for (int i = 0; i < num_splits_between_cascades; i++) {
		if (cascade_plane_distances[i] > world_depth_value) {
			layer = i;
			break;
		}
	}

	return get_csm_shadow_from_layer(uint(layer), fragment_pos_world_space);
}
