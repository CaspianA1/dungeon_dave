#version 330 core

#include "common/shadow.frag"

flat in uint face_id;
in vec3 UV, fragment_pos_world_space;

out vec3 color;

uniform bool enable_tone_mapping;

uniform float
	ambient, diffuse_strength,
	specular_strength, exposure, noise_granularity;

uniform vec2 specular_exponent_domain, one_over_screen_size, UV_translation;

uniform vec3 camera_pos_world_space, dir_to_light, light_color, UV_translation_area[2];

uniform sampler2DArray texture_sampler, normal_map_sampler;

float diffuse(vec3 fragment_normal) {
	float diffuse_amount = dot(fragment_normal, dir_to_light);
	return diffuse_strength * max(diffuse_amount, 0.0f);
}

float specular(vec3 texture_color, vec3 fragment_normal) {
	/* Brighter texture colors have more specularity, and stronger highlights.
	Also, the specular calculation uses Blinn-Phong, rather than just Phong. */

	vec3 view_dir = normalize(camera_pos_world_space - fragment_pos_world_space);
	vec3 halfway_dir = normalize(dir_to_light + view_dir);
	float cos_angle_of_incidence = max(dot(fragment_normal, halfway_dir), 0.0f);

	//////////

	const float one_third = 1.0f / 3.0f;
	float texture_color_strength = (texture_color.r + texture_color.g + texture_color.b) * one_third;
	float specular_exponent = mix(specular_exponent_domain.x, specular_exponent_domain.y, texture_color_strength);
	return specular_strength * texture_color_strength * pow(cos_angle_of_incidence, specular_exponent);
}

vec3 calculate_light(vec3 texture_color, vec3 fragment_normal) {
	float non_ambient = diffuse(fragment_normal) + specular(texture_color, fragment_normal);
	float light_strength = ambient + non_ambient * shadow();
	return light_strength * light_color * texture_color;
}

vec3 get_fragment_normal(vec3 offset_UV) {
	// `t` = tangent space normal. Normalized b/c linear filtering may unnormalize it.
	vec3 t = normalize(texture(normal_map_sampler, offset_UV).rgb * 2.0f - 1.0f);

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

vec3 postprocess_light(vec3 color) {
	// HDR through tone mapping
	vec3 tone_mapped_color = vec3(1.0f) - exp(-color * exposure);
	color = mix(color, tone_mapped_color, float(enable_tone_mapping));

	// Noise is added to remove color banding
	vec2 screen_fragment_pos = gl_FragCoord.xy * one_over_screen_size;
	float random_value = fract(sin(dot(screen_fragment_pos, vec2(12.9898f, 78.233f))) * 43758.5453f);
	return color + mix(-noise_granularity, noise_granularity, random_value);
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
