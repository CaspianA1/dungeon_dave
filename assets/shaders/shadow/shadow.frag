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

	/* If the previous UV is out of range, and the current one is in range,
	there should be no blending between shadow layer values */
	bool no_layer_blending =
		prev_fragment_pos_light_space != clamp(prev_fragment_pos_light_space, 0.0f, 1.0f) &&
		curr_fragment_pos_light_space == clamp(curr_fragment_pos_light_space, 0.0f, 1.0f);

	in_light_percentages.x = no_layer_blending ? in_light_percentages.y : in_light_percentages.x;

	///////////

	return clamp(in_light_percentages, 0.0f, 1.0f);
}

float get_volumetric_light_from_layer( const uint layer_index, const vec3 fragment_pos_world_space, const vec3 view_dir) {
	/* TODO:
	- Find a way to make this lighting scheme work with my current lighting equation
		- The question: given a light source, and a light color, how do we use an illumination
			equation to fit the god ray term in.
		- Idea for that: multiply the ambient term by the god ray term? Hm, doesn't quite work.
		- A good reference point for nice looking volumetric lighting chould be Quake 2 RTX.
		- See this video on volumetric lighting: https://www.youtube.com/watch?v=G0sYTrX3VHI

	- God rays seems to disappear mostly after the first cascade (it could have to do with the
		camera light space transform - the camera would only be in bounds for the first cascade)
	- Decide if I should cut out light scattering beyond cascade level 0 -
		if so, I can precompute some things, like the camera and fragment positions in cascade space
	- Very close occluder receiver diffs become messed up
	- Some volumes seem to go into the ground
	- Add a volumetricity strength param too (stronger -> more samples, and more pronounced volumes)

	- Figure out proper blending between cascades
	- Find out if only calculating the god ray value for the prev layer index is actually feasible

	- Perhaps multiply the god ray value by values in a 3D Perlin noise map eventually (or for fog, by some fog map)
	- Different volumetric effects: mosly uniform volume, fog, etc.
	- Fog would be so cool - find some way to make a volumetric fog function for a given world-space position, and a seed like time.
	- Dust would be really cool as well, and work well for the desert environment too
	- A particle system could be cool with this
	- An interesting presentation about volumetric rendering: https://www.ea.com/frostbite/news/physically-based-unified-volumetric-rendering-in-frostbite

	- Higher sample densities (e.g. 4.0) leads to god rays being shown that should be occluded
	*/

	// TODO: put these in the shared shading params
	const float
		decay = 0.9f,
		decay_weight = 0.7f,
		sample_density = 0.5f,
		opacity = 0.015f;

		/*
		max_poisson_texel_range = 0.75f,
		noise_scale = 10.0f;
		*/

	const uint num_samples = 25;
	const bool enabled = true;

	if (!enabled) return 0.0f;

	////////// Poisson disk setup

	/*
	const vec2 poisson_disk[] = vec2[](
		vec2(-0.94201624f, -0.39906216f),
		vec2(0.94558609f, -0.76890725f),
		vec2(-0.094184101f, -0.9293887f),
		vec2(0.34495938f, 0.2938776f),
		vec2(-0.91588581f, 0.45771432f),
		vec2(-0.81544232f, -0.87912464f),
		vec2(-0.38277543f, 0.27676845f),
		vec2(0.97484398f, 0.75648379f),
		vec2(0.44323325f, -0.97511554f),
		vec2(0.53742981f, -0.4737342f),
		vec2(-0.26496911f, -0.41893023f),
		vec2(0.79197514f, 0.19090188f),
		vec2(-0.24188840f, 0.99706507f),
		vec2(-0.81409955f, 0.9143759f),
		vec2(0.19984126f, 0.78641367f),
		vec2(0.14383161f, -0.1410079f)
	);
	*/

	/* If the camera less aligned towards a shadow map
	texel (which is projected in the direction of the light),
	more projective aliasing will be apparent. So, when there's more projective aliasing,
	the jitter range for the Poisson disk will be greater, which trades noise for banding. */

	// TODO: don't recalculate
	/*
	float projective_error = 1.0f - abs(dot(view_dir, dir_to_light));
	vec2 texel_size = 1.0f / textureSize(shadow_cascade_sampler, 0).xy;
	vec2 poisson_jitter_range = texel_size * (max_poisson_texel_range * projective_error);
	*/

	//////////

	mat4 light_view_projection_matrix = light_view_projection_matrices[layer_index];

	vec3 // TODO: don't recalculate `fragment_pos_cascade_space`
		camera_pos_cascade_space = (light_view_projection_matrix * vec4(camera_pos_world_space, 1.0f)).xyz * 0.5f + 0.5f,
		fragment_pos_cascade_space = (light_view_projection_matrix * vec4(fragment_pos_world_space, 1.0f)).xyz * 0.5f + 0.5f;

	const float sample_density_over_sample_count = sample_density / num_samples;

	vec3
		delta_UV = (camera_pos_cascade_space - fragment_pos_cascade_space) * sample_density_over_sample_count,
		curr_pos_cascade_space = camera_pos_cascade_space;

	float curr_decay = decay_weight, volumetric_light_strength = 0.0f;

	//////////

	for (uint i = 0; i < num_samples; i++) {
		curr_pos_cascade_space -= delta_UV;

		////////// Jittering that UV

		// TODO: make this seed account for 3 dimensions in a better way (a 3D rand function, probably)
		/*
		vec2 rand_seed = curr_pos_cascade_space.xy + curr_pos_cascade_space.z;
		float rand_val = fract(sin(dot(rand_seed, vec2(12.9898f, 78.233f))) * 43758.5453f);

		// The bitwise `and` is valid since the length of the array is a power of 2
		uint index = uint(rand_val * noise_scale) & uint(poisson_disk.length() - 1u);
		*/

		/* The jitter range is not baked into the Poisson disk array for a somewhat technical reason.
		If I do so, each local array will be different, which means that it will be stored within each thread's
		local registers. I then run out of local registers fairly quickly, which is a big slowdown (at least on my system). */
		// vec2 jittered_pos_cascade_space = poisson_disk[index] * poisson_jitter_range + curr_pos_cascade_space.xy;

		//////////

		float depth_test = texture(shadow_cascade_sampler_depth_comparison,
			// vec4(jittered_pos_cascade_space, layer_index, curr_pos_cascade_space.z)
			vec4(curr_pos_cascade_space.xy, layer_index, curr_pos_cascade_space.z)
		);

		volumetric_light_strength += depth_test * curr_decay;
		curr_decay *= decay;
	}

	return opacity * min(volumetric_light_strength, 1.0f);
}

// The first component of the return value equals the shadow strength, and the second one equals the volumetric light value.
vec2 get_csm_shadow_and_volumetric_light(const vec3 fragment_pos_world_space, const vec3 view_dir) {
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

	float
		shadow = mix(prev_and_curr.x, prev_and_curr.y, percent_between),
		volumetric_light = get_volumetric_light_from_layer(prev_layer_index, fragment_pos_world_space, view_dir);

	/* Sampling from the previous layer for volumetric lighting seems
	to give better results than the current layer, oddly enough */

	return vec2(shadow, volumetric_light);
}
