#version 400 core

in vec3 fragment_pos_light_space;

uniform int pcf_radius;
uniform float esm_constant;
uniform sampler2D shadow_map_sampler;

float shadow(void) {
	/* Notes:
	- Lighter shadow bases are simply an artifact of ESM
	- A higher exponent means more shadow acne, but less light bleeding */

	vec2
		shadow_map_UV = fragment_pos_light_space.xy,
		texel_size = 1.0f / textureSize(shadow_map_sampler, 0);

	float average_occluder_depth = 0.0f;

	for (int y = -pcf_radius; y <= pcf_radius; y++) {
		for (int x = -pcf_radius; x <= pcf_radius; x++) {
			vec2 sample_UV = shadow_map_UV + texel_size * vec2(x, y);
			average_occluder_depth += texture(shadow_map_sampler, sample_UV).r;
		}
	}

	int samples_across = (pcf_radius << 1) + 1;
	average_occluder_depth /= samples_across * samples_across;

	//////////

	float occluder_receiver_diff = fragment_pos_light_space.z - average_occluder_depth;
	float in_light_percentage = exp(-esm_constant * occluder_receiver_diff);
	return clamp(in_light_percentage, 0.0f, 1.0f);
}
