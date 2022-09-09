#version 400 core

#include "shared_params.glsl"
#include "UV_utils.frag"
#include "shadow/shadow.frag"

in vec3 fragment_pos_world_space, UV, ambient_occlusion_UV;

// TODO: set these uniforms through a shared fn for world-shaded objects
uniform samplerCube environment_map_sampler;
uniform sampler2DArray diffuse_sampler, normal_map_sampler;
uniform sampler3D ambient_occlusion_sampler;

float diffuse(const vec3 fragment_normal) {
	float diffuse_amount = dot(fragment_normal, dir_to_light);
	return strengths.diffuse * max(diffuse_amount, 0.0f);
}

vec3 specular(const vec3 texture_color, const vec3 fragment_normal) {
	/* Brighter texture colors have more specularity, and stronger highlights.
	Also, the specular calculation uses Blinn-Phong, rather than just Phong.
	The specular strength is also multiplied by the corresponding value in an
	environment map (which is actually just the skybox under the hood), in order
	to factor in the general environment's light into this calculation. */

	vec3 view_dir = normalize(camera_pos_world_space - fragment_pos_world_space);
	vec3 halfway_dir = normalize(dir_to_light + view_dir);
	float cos_angle_of_incidence = max(dot(fragment_normal, halfway_dir), 0.0f);

	//////////

	const float one_third = 1.0f / 3.0f;
	float texture_color_strength = (texture_color.r + texture_color.g + texture_color.b) * one_third;
	float specular_exponent = mix(specular_exponent_domain.x, specular_exponent_domain.y, texture_color_strength);
	float specular_value = strengths.specular * pow(cos_angle_of_incidence, specular_exponent);

	//////////

	vec3 reflection_dir = reflect(-view_dir, fragment_normal);
	reflection_dir.x = -reflection_dir.x;
	vec3 env_map_value = texture(environment_map_sampler, reflection_dir).rgb;

	return specular_value * env_map_value;
}

float get_ao_strength(void) {
	float unmodulated_ao_strength = texture(ambient_occlusion_sampler, ambient_occlusion_UV).r;
	return mix(1.0f, unmodulated_ao_strength, percents.ao);
}

// When the shadow layer is already known (like for the weapon sprite), this can be useful to call
vec4 calculate_light_with_provided_shadow_strength(const float shadow_strength, const vec3 UV, const vec3 fragment_normal) {
	vec3 adjusted_UV = UV;
	adjust_UV_for_pixel_art_filtering(percents.bilinear, textureSize(diffuse_sampler, 0).xy, adjusted_UV.xy);

	vec4 texture_color = texture(diffuse_sampler, adjusted_UV);

	float ao_strength = get_ao_strength();

	// return vec4(vec3(ao_strength), 1.0f);
	// ao_strength = 1.0f;

	vec3 non_ambient = diffuse(fragment_normal) + specular(texture_color.rgb, fragment_normal);
	vec3 light_strength = (non_ambient * shadow_strength + strengths.ambient) * ao_strength;

	return vec4(light_strength * texture_color.rgb * overall_scene_tone, texture_color.a);
}

vec4 calculate_light(const float world_depth_value, const vec3 UV, const vec3 fragment_normal) {
	float shadow_strength = get_csm_shadow(world_depth_value, fragment_pos_world_space);
	return calculate_light_with_provided_shadow_strength(shadow_strength, UV, fragment_normal);
}

// https://64.github.io/tonemapping/ (Reinhard Extended Luminance)
vec3 apply_tone_mapping(const vec3 color, const float max_white) {
	float old_luminance = dot(color, vec3(0.2126f, 0.7152f, 0.0722f));
	float numerator = old_luminance * (1.0f + (old_luminance / (max_white * max_white)));
	float new_luminance = numerator / (1.0f + old_luminance);
	return color * (new_luminance / old_luminance);
}

vec3 noise_for_banding_removal(const vec2 seed, const vec3 color) {
	float random_value = fract(sin(dot(seed, vec2(12.9898f, 78.233f))) * 43758.5453f);
	return color + mix(-noise_granularity, noise_granularity, random_value);
}

vec3 postprocess_light(const vec2 UV, const vec3 color) {
	vec3 tone_mapped_color = apply_tone_mapping(color, tone_mapping_max_white);
	return noise_for_banding_removal(UV, tone_mapped_color);
}
