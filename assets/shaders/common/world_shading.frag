#version 400 core

#include "shared_params.glsl"
#include "UV_utils.frag"
#include "normal_utils.frag"
#include "parallax_mapping.frag"
#include "sample_ambient_occlusion.frag"
#include "../shadow/shadow.frag"

flat in uint material_index, bilinear_percents_index;
in vec3 UV, fragment_pos_world_space, camera_to_fragment_world_space;
flat in mat3 fragment_tbn;

// These are set through a shared fn for world-shaded objects
uniform samplerBuffer materials_sampler;
uniform sampler2DArray albedo_sampler, normal_sampler;

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

////////// PBR

const float PI = 3.14159265359f;
const float ONE_OVER_PI = 1.0f / PI;

// Fresnel-Schlick; an approximation to the Fresnel effect.
vec3 fresnel_schlick(const float h_dot_v, const vec3 F0) {
	return F0 + (1.0f - F0) * pow(1.0f - h_dot_v, 5.0f);
}

// Trowbridge-Reitz GGX normal distribution function.
float normal_distribution(const float roughness, const float n_dot_h) {
	float alpha = roughness * roughness;
	float alpha_squared = alpha * alpha;
	float denominator = (n_dot_h * n_dot_h) * (alpha_squared - 1.0f) + 1.0f;

	denominator = PI * (denominator * denominator);
	return alpha_squared / denominator;
}

// Smith's function gives an estimation to how many local microfacets are self-shadowing.
float geometry_smith(const float roughness, const float n_dot_v, const float n_dot_l) {
	float r = roughness + 1.0f;
	float k = (r * r) / 8.0f;
	float one_minus_k = 1.0f - k;

	// Using the Schlick-GGX geometry function
	vec2 dots = vec2(n_dot_v, n_dot_l);
	vec2 ggx_results = dots / (dots * one_minus_k + k);

	return ggx_results.x * ggx_results.y;
}

vec4 calculate_light() {
	const float almost_zero = 0.0001f;
	const vec3 dielectric_F0 = vec3(0.04f);

	vec3 lighting_properties = texelFetch(materials_sampler, int(material_index)).rgb;
	vec2 bilinear_percents = all_bilinear_percents[bilinear_percents_index];

	float
		metallicity = lighting_properties[0],
		min_roughness = lighting_properties[1],
		max_roughness = lighting_properties[2],
		albedo_bilinear_percent = bilinear_percents[0],
		normal_bilinear_percent = bilinear_percents[1];

	//////////

	vec3 parallax_UV_for_normal = get_parallax_UV(UV, normal_sampler);
	vec3 parallax_UV_for_albedo = parallax_UV_for_normal;

	/* Parallax UVs for albedo and normal have different bilinear percents, so this adjusts
	those individually. Note that the bilinear percent for the parallax heightmap is always 1. */
	adjust_UV_for_pixel_art_filtering(albedo_bilinear_percent, textureSize(albedo_sampler, 0).xy, parallax_UV_for_albedo.xy);
	adjust_UV_for_pixel_art_filtering(normal_bilinear_percent, textureSize(normal_sampler, 0).xy, parallax_UV_for_normal.xy);

	vec4
		albedo = texture(albedo_sampler, parallax_UV_for_albedo),
		normal_and_inv_height = get_tangent_space_normal_3D(normal_sampler, parallax_UV_for_normal);

	/* If an object's heightmap (built from its albedo texture) has steeper height changes,
	its surface normals will be more slanted, and so the Z components of those normals
	will be more aligned with the plane. So here, I am interpolating between the max and min
	roughness, based on the magnitude of the local height change on the normal map. */
	float roughness = mix(max_roughness, min_roughness, normal_and_inv_height.z);

	////////// https://learnopengl.com/PBR/Lighting and https://www.youtube.com/watch?v=5p0e7YNONr8

	vec3
		fragment_normal = fragment_tbn * normal_and_inv_height.xyz,
		view_dir = normalize(camera_to_fragment_world_space);

	vec3 halfway_dir = normalize(dir_to_light + view_dir);

	float
		n_dot_v = max(dot(fragment_normal, view_dir), almost_zero),
		n_dot_l = max(dot(fragment_normal, dir_to_light), almost_zero),
		h_dot_v = max(dot(halfway_dir, view_dir), 0.0f),
		n_dot_h = max(dot(fragment_normal, halfway_dir), 0.0f);

	////////// See more PBR sub-algorithms here: https://www.jordanstevenstechart.com/physically-based-rendering

	float
		D = normal_distribution(roughness, n_dot_h),
		G = geometry_smith(roughness, n_dot_v, n_dot_l);

	vec3 F0 = mix(dielectric_F0, albedo.rgb, metallicity);
	vec3 F = fresnel_schlick(h_dot_v, F0);

	vec3
		specular = (D * G * F) / (4.0f * n_dot_v * n_dot_l),
		diffuse = (1.0f - F) * (1.0f - metallicity);

	vec2 shadow_and_volumetric_light = get_csm_shadow_and_volumetric_light(fragment_pos_world_space);

	//////////

	vec3 radiance = light_color; // TODO: do some variant of IBL to add the environment map sampler back in
	vec3 Lo = (diffuse * albedo.rgb * ONE_OVER_PI + specular) * radiance * n_dot_l * shadow_and_volumetric_light.x;

	vec3 ambient = get_ambient_strength(fragment_pos_world_space) * albedo.rgb;
	vec3 color = ((ambient + Lo) + (light_color * shadow_and_volumetric_light.y)) * albedo.a;

	apply_tone_mapping(tone_mapping_max_white, color);
	apply_noise_for_banding_removal(UV.xy, color);

	return vec4(color, albedo.a);
}
