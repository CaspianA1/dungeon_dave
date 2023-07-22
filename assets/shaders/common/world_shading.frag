#version 400 core

#include "shared_params.glsl"
#include "UV_utils.frag"
#include "parallax_mapping.frag"
#include "sample_ambient_occlusion.frag"
#include "../shadow/shadow.frag"

#include "pbr.frag"

flat in uint material_index, bilinear_percents_index;
in vec3 UV, fragment_pos_world_space, camera_to_fragment_world_space;
flat in mat3 fragment_tbn;

out vec4 color;

// These are set through a shared function for world-shaded objects
uniform samplerBuffer materials_sampler;
uniform sampler2DArray albedo_sampler, normal_sampler;

void main(void) {
	vec4 albedo_color;
	vec3 tangent_space_normal;

	get_albedo_and_normal(albedo_sampler, normal_sampler,
		all_bilinear_percents[bilinear_percents_index],
		get_parallax_UV(UV, normal_sampler), albedo_color,
		tangent_space_normal);

	//////////

	vec2 noise_seed = UV.xy;
	vec3 lighting_properties = texelFetch(materials_sampler, int(material_index)).rgb;

	color = calculate_lighting(
		fragment_tbn,
		tangent_space_normal,
		camera_to_fragment_world_space,

		get_csm_shadow_and_volumetric_light(fragment_pos_world_space),
		lighting_properties, light_color, dir_to_light, albedo_color,

		get_ambient_strength(fragment_pos_world_space),
		tone_mapping_max_white, noise_granularity, noise_seed
	);
}
