#version 330 core

in vec3 UV, fragment_pos_light_space;
out vec4 color;

uniform int pcf_radius;
uniform float ambient, esm_constant;
uniform sampler2D shadow_map_sampler;
uniform sampler2DArray texture_sampler;

// TODO: share this function between the sector and billboard fragment shaders
float shadow(void) {
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
	average_occluder_depth *= 1.0f / (samples_across * samples_across);

	//////////

	float occluder_receiver_diff = fragment_pos_light_space.z - average_occluder_depth;
	float in_light_percentage = exp(-esm_constant * occluder_receiver_diff);
	return clamp(in_light_percentage, 0.0f, 1.0f);
}

void main(void) {
	color = texture(texture_sampler, UV);
	color.rgb *= mix(ambient, 1.0f, shadow());
}
