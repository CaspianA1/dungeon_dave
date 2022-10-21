#version 400 core

#include "shared_params.glsl"
#include "UV_utils.frag"
#include "normal_utils.frag"
#include "parallax_mapping.frag"
#include "ambient_occlusion.frag"
#include "../shadow/shadow.frag"

in vec3 fragment_pos_world_space, UV;
flat in mat3 fragment_tbn;

// These are set through a shared fn for world-shaded objects
uniform samplerCube environment_map_sampler;
uniform sampler2DArray diffuse_sampler, normal_map_sampler;

float diffuse(const vec3 fragment_normal) { // Lambert
	float diffuse_amount = dot(fragment_normal, dir_to_light);
	return strengths.diffuse * max(diffuse_amount, 0.0f);
}

vec3 specular(const vec4 normal_and_inv_height) { // Blinn-Phong
	vec3 view_dir = normalize(camera_pos_world_space - fragment_pos_world_space);
	vec3 halfway_dir = normalize(dir_to_light + view_dir);
	float cos_angle_of_incidence = max(dot(normal_and_inv_height.xyz, halfway_dir), 0.0f);

	//////////

	float local_roughness = fwidth(normal_and_inv_height.a); // Greater heightmap change -> more local roughness

	// TOOD: add a specular exponent strength param per object instance
	float specular_exponent = mix(specular_exponents.matte, specular_exponents.rough, local_roughness);
	float specular_value = strengths.specular * pow(cos_angle_of_incidence, specular_exponent);

	//////////

	vec3 reflection_dir = reflect(-view_dir, normal_and_inv_height.xyz);
	vec3 env_map_value = texture(environment_map_sampler, reflection_dir).rgb;

	// More of the environment map will be reflected for less rough surfaces
	env_map_value = mix(env_map_value, vec3(1.0f), local_roughness);

	//////////

	return specular_value * env_map_value;
}

// https://64.github.io/tonemapping/ (Reinhard Extended Luminance)
void apply_tone_mapping(const float max_white, inout vec3 color) {
	float old_luminance = dot(color, vec3(0.2126f, 0.7152f, 0.0722f));
	float numerator = old_luminance * (1.0f + (old_luminance / (max_white * max_white)));
	float new_luminance = numerator / (1.0f + old_luminance);
	color *= (new_luminance / old_luminance);
}

void apply_noise_for_banding_removal(const vec2 seed, inout vec3 color) {
	float random_value = fract(sin(dot(seed, vec2(12.9898f, 78.233f))) * 43758.5453f);
	color += mix(-noise_granularity, noise_granularity, random_value);
}

vec4 calculate_light(void) {
	vec3 parallax_UV_for_diffuse = get_parallax_UV(UV, normal_map_sampler);
	vec3 parallax_UV_for_normal = parallax_UV_for_diffuse;

	// Parallax UVs for diffuse and normal have different bilinear percents, so this adjusts those individually
	adjust_UV_for_pixel_art_filtering(bilinear_percents.diffuse, textureSize(diffuse_sampler, 0).xy, parallax_UV_for_diffuse.xy);
	adjust_UV_for_pixel_art_filtering(bilinear_percents.normal, textureSize(normal_map_sampler, 0).xy, parallax_UV_for_normal.xy);

	vec4 normal_and_inv_height = get_tangent_space_normal_3D(normal_map_sampler, parallax_UV_for_normal);
	normal_and_inv_height.xyz = fragment_tbn * normal_and_inv_height.xyz;

	// return vec4(specular(normal_and_inv_height), 1.0f);
	// return vec4(vec3(get_ao_strength()), 1.0f);
	// return vec4(vec3(diffuse(normal_and_inv_height.xyz)), 1.0f);

	vec3 non_ambient = diffuse(normal_and_inv_height.xyz) + specular(normal_and_inv_height);
	vec3 light_strength = non_ambient * get_csm_shadow(fragment_pos_world_space) + strengths.ambient * get_ao_strength();

	vec4 texture_color = texture(diffuse_sampler, parallax_UV_for_diffuse);
	vec4 color = vec4(texture_color.rgb * light_strength * overall_scene_tone, texture_color.a);

	apply_tone_mapping(tone_mapping_max_white, color.rgb);
	apply_noise_for_banding_removal(UV.xy, color.rgb);

	return color;
}
