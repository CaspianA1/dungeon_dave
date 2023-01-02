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

float get_csm_shadow_from_layers(const uint prev_layer_index, const uint curr_layer_index, const vec3 fragment_pos_world_space) {
	/////////// Defining some shared vars

	vec2 texel_size = 1.0f / textureSize(shadow_cascade_sampler, 0).xy;
	vec2 sample_extent = shadow_mapping.sample_radius * texel_size;
	uint samples_per_row = (shadow_mapping.sample_radius << 1u) + 1u;
	float one_over_samples_per_kernel = 1.0f / (samples_per_row * samples_per_row);

	vec3 curr_fragment_pos_cascade_space = (light_view_projection_matrices[curr_layer_index]
		* vec4(fragment_pos_world_space, 1.0f)).xyz * 0.5f + 0.5f;

	vec3 curr_sample_UV = vec3(curr_fragment_pos_cascade_space.xy - sample_extent, curr_layer_index);

	/////////// A shadow-computing macro

	#define COMPUTE_SHADOW(type_t, UPDATE_Y, UPDATE_X, TEXTURE_SAMPLES,\
		SAMPLE_UV_RESET, FRAGMENT_POSITIONS_CASCADE_SPACE, ESM_EXPONENT_SCALING)\
		\
		type_t occluder_depth_sum = type_t(0.0f);\
		for (uint y = 0; y < samples_per_row; y++, UPDATE_Y) {\
			for (uint x = 0; x < samples_per_row; x++, UPDATE_X)\
				occluder_depth_sum += type_t TEXTURE_SAMPLES;\
			SAMPLE_UV_RESET\
		}\
		type_t occluder_receiver_diff = occluder_depth_sum * one_over_samples_per_kernel - type_t FRAGMENT_POSITIONS_CASCADE_SPACE;\
		type_t layer_scaled_esm_exponent = shadow_mapping.esm_exponent * ESM_EXPONENT_SCALING;\
		type_t in_light_percentage = clamp(exp(layer_scaled_esm_exponent * occluder_receiver_diff), 0.0f, 1.0f);

	/////////// If the current layer index is non-zero, we should blend the curr and last layer's shadows

	if (curr_layer_index != 0u) {
		vec3 prev_fragment_pos_cascade_space = (light_view_projection_matrices[prev_layer_index]
			* vec4(fragment_pos_world_space, 1.0f)).xyz * 0.5f + 0.5f;

		vec3 prev_sample_UV = vec3(prev_fragment_pos_cascade_space.xy - sample_extent, prev_layer_index);

		COMPUTE_SHADOW(vec2,
			(prev_sample_UV.y += texel_size.y, curr_sample_UV.y += texel_size.y),
			(prev_sample_UV.x += texel_size.x, curr_sample_UV.x += texel_size.x),

			(texture(shadow_cascade_sampler, prev_sample_UV).r, texture(shadow_cascade_sampler, curr_sample_UV).r),

			prev_sample_UV.x = prev_fragment_pos_cascade_space.x - sample_extent.x;
			curr_sample_UV.x = curr_fragment_pos_cascade_space.x - sample_extent.x;,

			(prev_fragment_pos_cascade_space.z, curr_fragment_pos_cascade_space.z),
			pow(vec2(prev_layer_index, curr_layer_index) + 1.0f, vec2(shadow_mapping.esm_exponent_layer_scale_factor))
		)

		////////// Making layer blending useless if it shouldn't be used

		bool no_layer_blending = // Don't blend if the prev UV is out of range, and the curr one is in range
			prev_fragment_pos_cascade_space != clamp(prev_fragment_pos_cascade_space, 0.0f, 1.0f) &&
			curr_fragment_pos_cascade_space == clamp(curr_fragment_pos_cascade_space, 0.0f, 1.0f);

		in_light_percentage.x = no_layer_blending ? in_light_percentage.y : in_light_percentage.x;

		////////// Getting the percent between cascades, and using that to get a blended shadow value

		uint depth_range_shift = uint(curr_layer_index == NUM_CASCADE_SPLITS);

		float
			dist_ahead_of_last_split = world_depth_value - shadow_mapping.cascade_split_distances[prev_layer_index],
			depth_range = shadow_mapping.cascade_split_distances[curr_layer_index - depth_range_shift] -
				shadow_mapping.cascade_split_distances[prev_layer_index - depth_range_shift];

		// If it's the last layer index, `percent_between` may be more than 1
		float percent_between = min(dist_ahead_of_last_split / depth_range, 1.0f);
		return mix(in_light_percentage.x, in_light_percentage.y, percent_between);
	}
	else {
		/* If we're only in the first layer, only compute shadows for that one.
		This is done because the first layer covers a lot of screen real estate,
		so this can lead to a reasonable performance gain. */

		COMPUTE_SHADOW(float,
			(curr_sample_UV.y += texel_size.y),
			(curr_sample_UV.x += texel_size.x),

			(texture(shadow_cascade_sampler, curr_sample_UV).r),

			curr_sample_UV.x = curr_fragment_pos_cascade_space.x - sample_extent.x;,

			(curr_fragment_pos_cascade_space.z),
			1.0f
		)

		// TODO: account for out-of-frustum fragments in this branch
		return in_light_percentage;
	}
}

float get_volumetric_light_from_layer(const uint layer_index, const vec3 fragment_pos_world_space, const vec3 view_dir) {
	/* TODO:
	- Find a way to make this lighting scheme work with my current lighting equation
		- The question: given a light source, and a light color, how do we use an illumination
			equation to fit the god ray term in. Perhaps just consider it to be a blended medium?
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
	- For god rays that are shown over the skybox, perhaps go from screen-space to light-space
	*/

	if (volumetric_lighting.opacity == 0.0f) return 0.0f;

	mat4 light_view_projection_matrix = light_view_projection_matrices[layer_index];

	vec3 // TODO: don't recalculate `fragment_pos_cascade_space`
		camera_pos_cascade_space = (light_view_projection_matrix * vec4(camera_pos_world_space, 1.0f)).xyz * 0.5f + 0.5f,
		fragment_pos_cascade_space = (light_view_projection_matrix * vec4(fragment_pos_world_space, 1.0f)).xyz * 0.5f + 0.5f;

	float one_over_num_samples = 1.0f / volumetric_lighting.num_samples;
	float sample_density_over_sample_count = volumetric_lighting.sample_density * one_over_num_samples;

	vec3 // TODO: apply biasing by adding a small offset based on a light-space surface normal
		delta_UV = (camera_pos_cascade_space - fragment_pos_cascade_space) * sample_density_over_sample_count,
		curr_pos_cascade_space = fragment_pos_cascade_space;

	float volumetric_light_sum = 0.0f;

	//////////

	/* TODO:
	- Add some minor attenuation
	- Figure out a way to make the god rays weaker when closer to the viewer
	- Stop the ground from being so bright
	- See here: https://bartwronski.files.wordpress.com/2014/08/bwronski_volumetric_fog_siggraph2014.pdf
	Note: the results turn out weird sometimes because the opacity is outside of the [0, 1] domain
	*/

	// TODO: figure out a way to make the god rays become weaker when closer to the viewer
	for (uint i = 0; i < volumetric_lighting.num_samples; i++) {
		curr_pos_cascade_space += delta_UV;

		float light_percent_visiblity = texture(shadow_cascade_sampler_depth_comparison,
			vec4(curr_pos_cascade_space.xy, layer_index, curr_pos_cascade_space.z)
		);

		volumetric_light_sum += light_percent_visiblity;
	}

	float base_volumetric_light_strength = volumetric_light_sum * one_over_num_samples;
	return base_volumetric_light_strength * volumetric_lighting.opacity;
}

// The first component of the return value equals the shadow strength, and the second one equals the volumetric light value.
vec2 get_csm_shadow_and_volumetric_light(const vec3 fragment_pos_world_space, const vec3 view_dir) {
	uint layer_index = 0u; // TODO: calculate this in the vertex shader in some way - and just move more calculations to there

	while (layer_index < NUM_CASCADE_SPLITS
		&& shadow_mapping.cascade_split_distances[layer_index] <= world_depth_value)
		layer_index++;

	uint prev_layer_index = max(int(layer_index) - 1, 0);

	//////////

	return vec2( // TODO: perhaps pick a layer index based on the percent between?
		get_csm_shadow_from_layers(prev_layer_index, layer_index, fragment_pos_world_space),
		get_volumetric_light_from_layer(layer_index, fragment_pos_world_space, view_dir)
	);
}
