#version 400 core

in float world_depth_value;

uniform sampler2DArray shadow_cascade_sampler;
uniform sampler2DArrayShadow shadow_cascade_sampler_depth_comparison;

/* TODO:
Perhaps use Vogel disk or stratified Poisson sampling instead:
- https://www.gamedev.net/tutorials/programming/graphics/contact-hardening-soft-shadows-made-fast-r4906/
- http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/

ESM scaling:
- Bigger depth range will be darker, so scale the exponent primarily on that
- Secondarily, depth values will be different b/c depth values are normalized
	based on the cascade depth range, so also rescale also on the cascade
- Also, perhaps scale the esm exponent based on the overal layer depth range and percentage, rather than the layer index

Note: shadows are the major bottleneck with the terrain 2 level
*/

vec2 get_csm_shadows_from_layers(const uint prev_layer_index, const uint curr_layer_index, const vec3 fragment_pos_world_space) {
	// TODO: do an unvectorized version for the first layer, where there's no blending

	/////////// Defining some shared vars

	vec2 texel_size = 1.0f / textureSize(shadow_cascade_sampler, 0).xy;
	vec2 sample_extent = shadow_mapping.sample_radius * texel_size;
	uint samples_per_row = (shadow_mapping.sample_radius << 1u) + 1u;
	float one_over_samples_per_kernel = 1.0f / (samples_per_row * samples_per_row);

	/////////// Getting the average occluder depth for the prev layer

	vec2 occluder_depth_sums = vec2(0.0f);

	vec3 prev_fragment_pos_light_space = (light_view_projection_matrices[prev_layer_index]
		* vec4(fragment_pos_world_space, 1.0f)).xyz * 0.5f + 0.5f;

	vec3 curr_fragment_pos_light_space = (light_view_projection_matrices[curr_layer_index]
		* vec4(fragment_pos_world_space, 1.0f)).xyz * 0.5f + 0.5f;

	vec3
		prev_sample_UV = vec3(prev_fragment_pos_light_space.xy - sample_extent, prev_layer_index),
		curr_sample_UV = vec3(curr_fragment_pos_light_space.xy - sample_extent, curr_layer_index);

	for (uint y = 0; y < samples_per_row; y++, prev_sample_UV.y += texel_size.y, curr_sample_UV.y += texel_size.y) {
		for (uint x = 0; x < samples_per_row; x++, prev_sample_UV.x += texel_size.x, curr_sample_UV.x += texel_size.x) {
			occluder_depth_sums += vec2(
				texture(shadow_cascade_sampler, prev_sample_UV).r,
				texture(shadow_cascade_sampler, curr_sample_UV).r
			);
		}
		prev_sample_UV.x = prev_fragment_pos_light_space.x - sample_extent.x;
		curr_sample_UV.x = curr_fragment_pos_light_space.x - sample_extent.x;
	}

	/////////// Calculating the shadow strengths

	vec2 occluder_receiver_diffs = occluder_depth_sums * one_over_samples_per_kernel
		- vec2(prev_fragment_pos_light_space.z, curr_fragment_pos_light_space.z);

	vec2 layer_scaled_esm_exponents = shadow_mapping.esm_exponent * pow(
		vec2(prev_layer_index + 1u, curr_layer_index + 1u),
		vec2(shadow_mapping.esm_exponent_layer_scale_factor)
	);

	vec2 in_light_percentages = exp(layer_scaled_esm_exponents * occluder_receiver_diffs);

	/////////// Doing a sub-frustum bounds check

	/* If the previous UV is out of range, and the current one is in range,
	there should be no blending between shadow layer values */
	bool no_layer_blending =
		prev_fragment_pos_light_space != clamp(prev_fragment_pos_light_space, 0.0f, 1.0f) &&
		curr_fragment_pos_light_space == clamp(curr_fragment_pos_light_space, 0.0f, 1.0f);

	in_light_percentages.x = no_layer_blending ? in_light_percentages.y : in_light_percentages.x;

	///////////

	return clamp(in_light_percentages, 0.0f, 1.0f);
}

float get_volumetric_light_from_layer(const uint layer_index, const vec3 fragment_pos_world_space, const vec3 view_dir) {
	/* TODO:
	- Find a way to make this lighting scheme work with my current lighting equation
		- The question: given a light source, and a light color, how do we use an illumination
			equation to fit the god ray term in.
		- Idea for that: multiply the ambient term by the god ray term? Hm, doesn't quite work.
		- A good reference point for nice looking volumetric lighting chould be Quake 2 RTX.
		- See this video on volumetric lighting: https://www.youtube.com/watch?v=G0sYTrX3VHI

	- Find a way to stop self-shadowing for billboards
	- God rays seems to disappear mostly after the first cascade (it could have to do with the
		camera light space transform - the camera would only be in bounds for the first cascade)
	- Decide if I should cut out light scattering beyond cascade level 0 -
		if so, I can precompute some things, like the camera and fragment positions in cascade space
	- Very close occluder receiver diffs become messed up
	- Some volumes seem to go into the ground
	- Add a volumetricity strength param too (stronger -> more samples, and more pronounced volumes)

	- Perhaps multiply the god ray value by values in a 3D Perlin noise map eventually (or for fog, by some fog map)
	- Different volumetric effects: mosly uniform volume, fog, etc.
	- Fog would be so cool - find some way to make a volumetric fog function for a given world-space position, and a seed like time.
	- Dust would be really cool as well, and work well for the desert environment too
	- A particle system could be cool with this
	- An interesting presentation about volumetric rendering: https://www.ea.com/frostbite/news/physically-based-unified-volumetric-rendering-in-frostbite

	- Higher sample densities (e.g. 4.0) leads to god rays being shown that should be occluded
	*/

	if (!volumetric_lighting.enabled) return 0.0f;

	mat4 light_view_projection_matrix = light_view_projection_matrices[layer_index];

	vec3 // TODO: don't recalculate `fragment_pos_cascade_space`
		camera_pos_cascade_space = (light_view_projection_matrix * vec4(camera_pos_world_space, 1.0f)).xyz * 0.5f + 0.5f,
		fragment_pos_cascade_space = (light_view_projection_matrix * vec4(fragment_pos_world_space, 1.0f)).xyz * 0.5f + 0.5f;

	float sample_density_over_sample_count = volumetric_lighting.sample_density / volumetric_lighting.num_samples;

	vec3
		delta_UV = (camera_pos_cascade_space - fragment_pos_cascade_space) * sample_density_over_sample_count,
		curr_pos_cascade_space = camera_pos_cascade_space;

	float curr_decay = volumetric_lighting.decay_weight, volumetric_light_strength = 0.0f;

	//////////

	for (uint i = 0; i < volumetric_lighting.num_samples; i++) {
		curr_pos_cascade_space -= delta_UV;

		float depth_test = texture(shadow_cascade_sampler_depth_comparison,
			vec4(curr_pos_cascade_space.xy, layer_index, curr_pos_cascade_space.z)
		);

		volumetric_light_strength += depth_test * curr_decay;
		curr_decay *= volumetric_lighting.decay;
	}

	return volumetric_lighting.opacity * min(volumetric_light_strength, 1.0f);
}

// The first component of the return value equals the shadow strength, and the second one equals the volumetric light value.
vec2 get_csm_shadow_and_volumetric_light(const vec3 fragment_pos_world_space, const vec3 view_dir) {
	uint layer_index = 0u; // TODO: calculate this in the vertex shader in some way - and just move more calculations to there

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

	float
		shadow = mix(prev_and_curr.x, prev_and_curr.y, percent_between),
		volumetric_light = get_volumetric_light_from_layer(prev_layer_index, fragment_pos_world_space, view_dir);

	/* Sampling from the previous layer for volumetric lighting seems
	to give better results than the current layer, oddly enough */
	return vec2(shadow, volumetric_light);
}
