#version 400 core

// https://64.github.io/tonemapping/ (Reinhard Extended Luminance)
void apply_tone_mapping(const float max_white, inout vec3 color) {
	float old_luminance = dot(color, vec3(0.2126f, 0.7152f, 0.0722f));
	float numerator = old_luminance * (1.0f + (old_luminance / (max_white * max_white)));
	float new_luminance = numerator / (1.0f + old_luminance);
	color *= (new_luminance / old_luminance);
}

void apply_noise_for_banding_removal(const vec2 seed, inout vec3 color, const float noise_granularity) {
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

vec4 calculate_lighting(
	const mat3 fragment_tbn, const vec3 tangent_space_normal, const vec3 camera_to_fragment_world_space,

	const vec2 shadow_and_volumetric_light, const vec3 lighting_properties,
	const vec3 light_color, const vec3 dir_to_light, const vec4 albedo_color,

	const float ambient_strength, const float tone_mapping_max_white,
	const float noise_granularity, const vec2 noise_seed) {

	//////////

	const float almost_zero = 0.0001f;
	const vec3 dielectric_F0 = vec3(0.04f);

	float
		metallicity = lighting_properties[0],
		min_roughness = lighting_properties[1],
		max_roughness = lighting_properties[2];

	//////////

	/* If an object's heightmap (built from its albedo texture) has steeper height changes,
	its surface normals will be more slanted, and so the Z components of those normals
	will be more aligned with the plane. So here, I am interpolating between the max and min
	roughness, based on the magnitude of the local height change on the normal map. */
	float roughness = mix(max_roughness, min_roughness, tangent_space_normal.z);

	////////// https://learnopengl.com/PBR/Lighting, and https://www.youtube.com/watch?v=5p0e7YNONr8

	vec3
		fragment_normal = fragment_tbn * tangent_space_normal.xyz,
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

	vec3 F0 = mix(dielectric_F0, albedo_color.rgb, metallicity);
	vec3 F = fresnel_schlick(h_dot_v, F0);

	vec3
		specular = (D * G * F) / (4.0f * n_dot_v * n_dot_l),
		diffuse = (1.0f - F) * (1.0f - metallicity);

	//////////

	vec3 radiance = light_color; // TODO: do some variant of IBL to add the environment map sampler back in
	vec3 Lo = (diffuse * albedo_color.rgb * ONE_OVER_PI + specular) * radiance * n_dot_l * shadow_and_volumetric_light[0];

	vec3 ambient = ambient_strength * albedo_color.rgb;
	vec3 color = ((ambient + Lo) + (light_color * shadow_and_volumetric_light[1])) * albedo_color.a;

	apply_tone_mapping(tone_mapping_max_white, color);
	apply_noise_for_banding_removal(noise_seed, color, noise_granularity);

	return vec4(color, albedo_color.a);
}
