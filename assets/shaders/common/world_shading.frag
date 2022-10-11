#version 400 core

#include "shared_params.glsl"
#include "UV_utils.frag"
#include "normal_utils.frag"
#include "parallax_mapping.frag"
#include "../shadow/shadow.frag"

in vec3 fragment_pos_world_space, UV, ambient_occlusion_UV;

// These are set through a shared fn for world-shaded objects
uniform samplerCube environment_map_sampler;
uniform sampler2DArray diffuse_sampler, normal_map_sampler;
uniform sampler3D ambient_occlusion_sampler;

float diffuse(const vec3 fragment_normal) { // Lambert
	float diffuse_amount = dot(fragment_normal, dir_to_light);
	return strengths.diffuse * max(diffuse_amount, 0.0f);
}

vec3 specular(const vec3 texture_color, const vec4 normal_and_inv_height) { // Blinn-Phong
	vec3 view_dir = normalize(camera_pos_world_space - fragment_pos_world_space);
	vec3 halfway_dir = normalize(dir_to_light + view_dir);
	float cos_angle_of_incidence = max(dot(normal_and_inv_height.xyz, halfway_dir), 0.0f);

	//////////

	float roughness = fwidth(normal_and_inv_height.a); // Greater texture color change -> more local roughness
	float specular_exponent = mix(specular_exponents.matte, specular_exponents.rough, roughness);
	float specular_value = strengths.specular * pow(cos_angle_of_incidence, specular_exponent);

	//////////

	vec3 reflection_dir = reflect(-view_dir, normal_and_inv_height.xyz);
	reflection_dir.x = -reflection_dir.x;
	vec3 env_map_value = texture(environment_map_sampler, reflection_dir).rgb;

	// Not reading from the heightmap for this because texture brightness and normals should technically be decoupled
	const float one_third = 1.0f / 3.0f; // Brighter surfaces reflect more of the environment map
	float texture_brightness = (texture_color.r + texture_color.g + texture_color.b) * one_third;
	env_map_value = mix(vec3(1.0f), env_map_value, texture_brightness); // TODO: use Fresnel here instead

	//////////

	return specular_value * env_map_value;
}

float get_ao_strength(void) {
	float raw_ao_strength = texture(ambient_occlusion_sampler, ambient_occlusion_UV).r;

	/* The sRGB -> linear mapping is based on function from the bottom of here:
	https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml.
	It's done manually because there are no sRGB formats for 8-bit textures.
	TODO: move this calculation to the CPU. */

	float
		linear_for_below = raw_ao_strength / 12.92f,
		linear_for_above = pow((raw_ao_strength + 0.055f) / 1.055f, 2.5f);

	float linear_ao_strength = mix(linear_for_above, linear_for_below, float(raw_ao_strength <= 0.04045f));

	return mix(1.0f, linear_ao_strength, percents.ao);
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

// When the shadow layer is already known (like for the weapon sprite), this can be useful to call
vec4 calculate_light_with_provided_shadow_strength(const float shadow_strength, const vec3 UV, const mat3 tbn) {
	vec3 parallax_UV_for_diffuse = get_parallax_UV(UV, normal_map_sampler);
	vec3 parallax_UV_for_normal = parallax_UV_for_diffuse;

	// Parallax UVs for diffuse and normal have different bilinear percents, so this adjusts those individually
	adjust_UV_for_pixel_art_filtering(percents.bilinear_diffuse, textureSize(diffuse_sampler, 0).xy, parallax_UV_for_diffuse.xy);
	adjust_UV_for_pixel_art_filtering(percents.bilinear_normal, textureSize(normal_map_sampler, 0).xy, parallax_UV_for_normal.xy);

	vec4 texture_color = texture(diffuse_sampler, parallax_UV_for_diffuse);

	// return vec4(vec3(get_ao_strength()), 1.0f);
	// return vec4(vec3(diffuse(fragment_normal)), 1.0f);

	vec4 normal_and_inv_height = get_tangent_space_normal_3D(normal_map_sampler, parallax_UV_for_normal);
	normal_and_inv_height.xyz = tbn * normal_and_inv_height.xyz;

	vec3 non_ambient = diffuse(normal_and_inv_height.xyz) + specular(texture_color.rgb, normal_and_inv_height);
	vec3 light_strength = non_ambient * shadow_strength + strengths.ambient * get_ao_strength();

	vec4 color = vec4(light_strength * texture_color.rgb * overall_scene_tone, texture_color.a);

	apply_tone_mapping(tone_mapping_max_white, color.rgb);
	apply_noise_for_banding_removal(parallax_UV_for_diffuse.xy, color.rgb);

	return color;
}

vec4 calculate_light(const float world_depth_value, const vec3 UV, const mat3 tbn) {
	float shadow_strength = get_csm_shadow(world_depth_value, fragment_pos_world_space);
	return calculate_light_with_provided_shadow_strength(shadow_strength, UV, tbn);
}
