#version 400 core

#include "csm/shadow.frag"

flat in uint face_id;
in float ao_term, world_depth_value;
in vec3 UV, fragment_pos_world_space;

out vec3 color;

uniform bool enable_tone_mapping;

uniform float
	ambient, diffuse_strength, specular_strength, noise_granularity,

	tm_max_brightness, tm_linear_contrast, tm_linear_start, // `tm` = tone mapping
	tm_linear_length, tm_black, tm_pedestal;

uniform vec2 specular_exponent_domain, one_over_screen_size, UV_translation;
uniform vec3 camera_pos_world_space, light_dir, light_color, UV_translation_area[2];
uniform sampler2DArray texture_sampler, normal_map_sampler;

float diffuse(vec3 fragment_normal) {
	float diffuse_amount = dot(fragment_normal, light_dir);
	return diffuse_strength * max(diffuse_amount, 0.0f);
}

float specular(vec3 texture_color, vec3 fragment_normal) {
	/* Brighter texture colors have more specularity, and stronger highlights.
	Also, the specular calculation uses Blinn-Phong, rather than just Phong. */

	vec3 view_dir = normalize(camera_pos_world_space - fragment_pos_world_space);
	vec3 halfway_dir = normalize(light_dir + view_dir);
	float cos_angle_of_incidence = max(dot(fragment_normal, halfway_dir), 0.0f);

	//////////

	const float one_third = 1.0f / 3.0f;
	float texture_color_strength = (texture_color.r + texture_color.g + texture_color.b) * one_third;
	float specular_exponent = mix(specular_exponent_domain.x, specular_exponent_domain.y, texture_color_strength);
	return specular_strength * texture_color_strength * pow(cos_angle_of_incidence, specular_exponent);
}

vec3 calculate_light(vec3 texture_color, vec3 fragment_normal) {
	/*
	ao_term;
	float non_ambient = diffuse(fragment_normal) + specular(texture_color, fragment_normal);

	float light_strength = ambient + non_ambient * in_csm_shadow(world_depth_value, fragment_pos_world_space);
	return light_strength * light_color * texture_color;
	*/

	world_depth_value;
	return vec3(ao_term);
}

vec3 get_fragment_normal(vec3 UV) {
	// `t` = tangent space normal. Normalized b/c linear filtering may unnormalize it.
	vec3 t = normalize(texture(normal_map_sampler, UV).rgb * 2.0f - 1.0f);

	// No matrix multiplication here! :)
	vec3 rotated_vectors[5] = vec3[5](
		vec3(t.xz, -t.y), // Flat
		vec3(t.zy, -t.x), // Right
		t, // Bottom (equal to tangent space)
		vec3(-t.z, t.yx), // Left
		vec3(-t.x, t.y, -t.z) // Top (opposite of tangent space)
	);

	return rotated_vectors[face_id];
}

// https://github.com/dmnsgn/glsl-tone-map/blob/master/uchimura.glsl (but with better variable names)
vec3 uchimura_tone_mapping(vec3 color) {
	float L0 = ((tm_max_brightness - tm_linear_start) * tm_linear_length) / tm_linear_contrast;
	float S0 = tm_linear_start + L0, S1 = tm_linear_start + tm_linear_contrast * L0;
	float C2 = (tm_linear_contrast * tm_max_brightness) / (tm_max_brightness - S1);
	float CM = -C2 / tm_max_brightness;

	vec3
		W0 = vec3(1.0f - smoothstep(0.0f, tm_linear_start, color)),
		W2 = vec3(step(tm_linear_start + L0, color));

	vec3
		W1 = vec3(1.0f - W0 - W2),
		T = vec3(tm_linear_start * pow(color / tm_linear_start, vec3(tm_black)) + tm_pedestal),
		L = vec3(tm_linear_start + tm_linear_contrast * (color - tm_linear_start)),
		S = vec3(tm_max_brightness - (tm_max_brightness - S1) * exp(CM * (color - S0)));

	return (T * W0) + (L * W1) + (S * W2);
}

vec3 noise_for_banding_removal(vec3 color) {
	vec2 screen_fragment_pos = gl_FragCoord.xy * one_over_screen_size;
	float random_value = fract(sin(dot(screen_fragment_pos, vec2(12.9898f, 78.233f))) * 43758.5453f);
	return color + mix(-noise_granularity, noise_granularity, random_value);
}

vec3 postprocess_light(vec3 color) {
	color = mix(color, uchimura_tone_mapping(color), float(enable_tone_mapping));
	return noise_for_banding_removal(color);
}

/* Each level may have an area where sector UV
coordinates are re-translated for artistic purposes */
vec3 retranslate_UV(vec3 in_UV) {
	bvec3
		in_top_left_extent = bvec3(step(UV_translation_area[0], fragment_pos_world_space)),
		in_bottom_right_extent = bvec3(step(fragment_pos_world_space, UV_translation_area[1]));

	bool in_translation_area = in_top_left_extent == in_bottom_right_extent == true;

	in_UV.xy += UV_translation * float(in_translation_area);
	return in_UV;
}

void main(void) {
	vec3 translated_UV = retranslate_UV(UV);
	color = calculate_light(texture(texture_sampler, translated_UV).rgb, get_fragment_normal(translated_UV));
	color = postprocess_light(color);
}
