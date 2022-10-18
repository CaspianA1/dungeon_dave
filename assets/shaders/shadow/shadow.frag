#version 400 core

in float world_depth_value;

uniform sampler2DArray shadow_cascade_sampler;

/* TODO: perhaps use Vogel disk or stratified Poisson sampling instead:
- https://www.gamedev.net/tutorials/programming/graphics/contact-hardening-soft-shadows-made-fast-r4906/
- http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/ */
float get_average_occluder_depth(const uint sample_radius, const vec3 UV) {
	vec2 texel_size = 1.0f / textureSize(shadow_cascade_sampler, 0).xy;
	vec2 sample_extent = sample_radius * texel_size;
	vec3 sample_UV = vec3(UV.xy - sample_extent, UV.z);

	float average_occluder_depth = 0.0f;
	uint samples_per_row = (sample_radius << 1u) + 1u;

	for (uint y = 0; y < samples_per_row; y++, sample_UV.y += texel_size.y) {
		for (uint x = 0; x < samples_per_row; x++, sample_UV.x += texel_size.x)
			average_occluder_depth += texture(shadow_cascade_sampler, sample_UV).r;

		sample_UV.x = UV.x - sample_extent.x;
	}

	return average_occluder_depth / (samples_per_row * samples_per_row);
}

//////////

float get_csm_shadow_from_layer(const uint layer_index, const vec3 fragment_pos_world_space) {
	// TODO: put these in the shared params
	const uint sample_radius = 2;
	const float esm_constant = 45.0f, layer_scaling_component = 1.8f; // Palace
	// esm_constant = 80.0f, layer_scaling_component = 1.1f; // Terrain

	/* (TODO) ESM scaling:
	- Bigger depth range will be darker, so scale the exponent primarily on that
	- Secondarily, depth values will be different b/c depth values are normalized
		based on the cascade depth range, so also rescale also on the cascade
	- Also, perhaps scale the esm constant on the overall layer percentage,
		rather than the layer index
	*/

	/////////// Getting the distance between the occluder and the receiver

	vec3 fragment_pos_light_space = (light_view_projection_matrices[layer_index]
		* vec4(fragment_pos_world_space, 1.0f)).xyz * 0.5f + 0.5f;

	float avg_occluder_depth = get_average_occluder_depth(sample_radius, vec3(fragment_pos_light_space.xy, layer_index));
	float occluder_receiver_diff = avg_occluder_depth - fragment_pos_light_space.z;

	/////////// Calculating the shadow strength

	float layer_scaled_esm_constant = esm_constant * pow(layer_index + 1u, layer_scaling_component);
	float in_light_percentage = exp(layer_scaled_esm_constant * occluder_receiver_diff);
	return clamp(in_light_percentage, 0.0f, 1.0f);
}

float get_blended_csm_shadow(const uint layer_index, const uint depth_range_shift, const vec3 fragment_pos_world_space) {
	// If the layer index equals 0, this makes the previous layer 0 too
	uint prev_layer_index = max(int(layer_index) - 1, 0);

	float
		dist_ahead_of_last_split = world_depth_value - cascade_split_distances[prev_layer_index],
		depth_range = cascade_split_distances[layer_index - depth_range_shift] -
					cascade_split_distances[prev_layer_index - depth_range_shift];

	/* If the layer index equals 0, this will be less than 0; and if it's the last layer index,
	it may be over 1. This clamps the blend factor between 0 and 1 for when that happens. */
	float percent_between = clamp(dist_ahead_of_last_split / depth_range, 0.0f, 1.0f);

	return mix(
		get_csm_shadow_from_layer(prev_layer_index, fragment_pos_world_space),
		get_csm_shadow_from_layer(layer_index, fragment_pos_world_space),
		percent_between
	);
}

// TODO: don't pass around `fragment_pos_world_space` so much
float get_csm_shadow(const vec3 fragment_pos_world_space) {
	uint layer_index = 0;

	while (layer_index < NUM_CASCADE_SPLITS
		&& cascade_split_distances[layer_index] <= world_depth_value)
		layer_index++;

	bool on_last_split = layer_index == NUM_CASCADE_SPLITS;
	return get_blended_csm_shadow(layer_index, uint(on_last_split), fragment_pos_world_space);
}
