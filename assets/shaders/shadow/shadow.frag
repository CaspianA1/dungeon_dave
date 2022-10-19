#version 400 core

in float world_depth_value;

uniform sampler2DArray shadow_cascade_sampler;

/* TODO:
Perhaps use Vogel disk or stratified Poisson sampling instead:
- https://www.gamedev.net/tutorials/programming/graphics/contact-hardening-soft-shadows-made-fast-r4906/
- http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/

ESM scaling:
- Bigger depth range will be darker, so scale the exponent primarily on that
- Secondarily, depth values will be different b/c depth values are normalized
	based on the cascade depth range, so also rescale also on the cascade
- Also, perhaps scale the esm exponent based on the overal layer depth range and percentage, rather than the layer index
*/

vec2 get_csm_shadows_from_layers(const uint prev_layer_index, const uint curr_layer_index, const vec3 fragment_pos_world_space) {
	/////////// Defining some shared vars

	vec2 texel_size = 1.0f / textureSize(shadow_cascade_sampler, 0).xy;
	vec2 sample_extent = shadow_mapping.sample_radius * texel_size;
	uint samples_per_row = (shadow_mapping.sample_radius << 1u) + 1u;
	float one_over_samples_per_kernel = 1.0f / (samples_per_row * samples_per_row);

	/////////// Getting the average occluder depth for the prev layer

	vec3 prev_fragment_pos_light_space = (light_view_projection_matrices[prev_layer_index]
		* vec4(fragment_pos_world_space, 1.0f)).xyz * 0.5f + 0.5f;

	vec3 sample_UV = vec3(prev_fragment_pos_light_space.xy - sample_extent, prev_layer_index);
	float prev_occluder_depth_sum = 0.0f;

	for (uint y = 0; y < samples_per_row; y++, sample_UV.y += texel_size.y) {
		for (uint x = 0; x < samples_per_row; x++, sample_UV.x += texel_size.x)
			prev_occluder_depth_sum += texture(shadow_cascade_sampler, sample_UV).r;

		sample_UV.x = prev_fragment_pos_light_space.x - sample_extent.x;
	}

	/////////// Getting the average occluder depth for the curr layer

	vec3 curr_fragment_pos_light_space = (light_view_projection_matrices[curr_layer_index]
		* vec4(fragment_pos_world_space, 1.0f)).xyz * 0.5f + 0.5f;

	sample_UV = vec3(curr_fragment_pos_light_space.xy - sample_extent, curr_layer_index);
	float curr_occluder_depth_sum = 0.0f;

	for (uint y = 0; y < samples_per_row; y++, sample_UV.y += texel_size.y) {
		for (uint x = 0; x < samples_per_row; x++, sample_UV.x += texel_size.x)
			curr_occluder_depth_sum += texture(shadow_cascade_sampler, sample_UV).r;

		sample_UV.x = curr_fragment_pos_light_space.x - sample_extent.x;
	}

	/////////// Calculating the shadow strengths

	vec2 occluder_receiver_diffs =
		vec2(prev_occluder_depth_sum, curr_occluder_depth_sum)
		* one_over_samples_per_kernel
		- vec2(prev_fragment_pos_light_space.z, curr_fragment_pos_light_space.z);

	vec2 layer_scaled_esm_exponents = shadow_mapping.esm_exponent * pow(
		vec2(prev_layer_index + 1u, curr_layer_index + 1u),
		vec2(shadow_mapping.esm_exponent_layer_scale_factor)
	);

	vec2 in_light_percentages = exp(layer_scaled_esm_exponents * occluder_receiver_diffs);

	/////////// Doing a sub-frustum bounds check

	/* If the previous UV is out of range, and the current one
	is in range, there should be no blending between shadow layer values */
	bool no_layer_blending =
		prev_fragment_pos_light_space != clamp(prev_fragment_pos_light_space, 0.0f, 1.0f) &&
		curr_fragment_pos_light_space == clamp(curr_fragment_pos_light_space, 0.0f, 1.0f);

	in_light_percentages.x = no_layer_blending ? in_light_percentages.y : in_light_percentages.x;

	///////////

	return clamp(in_light_percentages, 0.0f, 1.0f);
}

float get_csm_shadow(const vec3 fragment_pos_world_space) {
	uint layer_index = 0;

	while (layer_index < NUM_CASCADE_SPLITS
		&& shadow_mapping.cascade_split_distances[layer_index] <= world_depth_value)
		layer_index++;

	//////////

	uint // If the layer index equals 0, this makes the previous layer 0 too
		depth_range_shift = uint(layer_index == NUM_CASCADE_SPLITS),
		prev_layer_index = max(int(layer_index) - 1, 0);

	float
		dist_ahead_of_last_split = world_depth_value - shadow_mapping.cascade_split_distances[prev_layer_index],
		depth_range = shadow_mapping.cascade_split_distances[layer_index - depth_range_shift] -
					shadow_mapping.cascade_split_distances[prev_layer_index - depth_range_shift];

	/* If the layer index equals 0, this will be less than 0; and if it's the last layer index,
	it may be over 1. This clamps the blend factor between 0 and 1 for when that happens. */
	float percent_between = clamp(dist_ahead_of_last_split / depth_range, 0.0f, 1.0f);

	vec2 prev_and_curr = get_csm_shadows_from_layers(prev_layer_index, layer_index, fragment_pos_world_space);
	return mix(prev_and_curr.x, prev_and_curr.y, percent_between);
}
