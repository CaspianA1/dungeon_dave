#version 400 core

#include "common/UV_utils.frag"
#include "common/pbr.frag"

noperspective in float scrolling_UV_x;
noperspective in vec2 UV;
noperspective in vec3 fragment_pos_tangent_space;

out vec3 color;

uniform float texture_transition_weight;

uniform vec2 scrolling_bilinear_percents;
uniform vec3 scrolling_light_pos_tangent_space;

uniform float tone_mapping_max_white, noise_granularity;
uniform vec3 still_and_scrolling_light_colors[2];
uniform vec4 still_and_scrolling_material_properties_and_ambient_strength[2];

uniform sampler2D still_albedo_sampler;
// These are only array textures so that they can work with `get_albedo_and_normal`.
uniform sampler2DArray scrolling_albedo_sampler, scrolling_normal_sampler;

//////////

void main(void) {
	const vec3 camera_pos_tangent_space = vec3(0.0f, 0.0f, 1.0f);

	const mat3 face_tbn = mat3(
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f
	);

	const vec2 shadow_and_volumetric_light = vec2(1.0f, 0.0f);

	////////// TODO: make these uniforms

	vec4 // TODO: why does increasing the ambient strength just a little ruin things so much for the scrolling texture?
		still_material_properties_and_ambient_strength = still_and_scrolling_material_properties_and_ambient_strength[0],
		scrolling_material_properties_and_ambient_strength = still_and_scrolling_material_properties_and_ambient_strength[1];

	////////// Getting the albedo and normal

	vec3 initial_scrolling_UV = vec3(scrolling_UV_x, UV.y, 0.0f);

	vec4 scrolling_albedo_color;
	vec3 scrolling_tangent_space_normal;

	get_albedo_and_normal(scrolling_albedo_sampler, scrolling_normal_sampler,
		scrolling_bilinear_percents, initial_scrolling_UV,
		scrolling_albedo_color, scrolling_tangent_space_normal);

	////////// Calculating dirs to lights

	vec3
		still_light_pos_tangent_space = scrolling_light_pos_tangent_space,
		camera_to_fragment_tangent_space = camera_pos_tangent_space - fragment_pos_tangent_space;

	// This makes the two different lights mirror each other, in terms of position
	// still_light_pos_tangent_space.x = -still_light_pos_tangent_space.x;

	vec3
		still_dir_to_light = normalize(still_light_pos_tangent_space - fragment_pos_tangent_space),
		scrolling_dir_to_light = normalize(scrolling_light_pos_tangent_space - fragment_pos_tangent_space);

	////////// Defining some last arbitrary vars

	vec4 still_albedo_color = texture(still_albedo_sampler, UV);
	still_albedo_color.a = scrolling_albedo_color.a = 1.0f;

	vec2 noise_seed = initial_scrolling_UV.xy;
	vec3 face_normal = face_tbn[2];

	////////// Shading

	vec3 still_color = calculate_lighting(
		face_tbn, face_normal, camera_to_fragment_tangent_space,

		shadow_and_volumetric_light, still_material_properties_and_ambient_strength.xyz,
		still_and_scrolling_light_colors[0], still_dir_to_light, still_albedo_color,

		still_material_properties_and_ambient_strength.w,
		tone_mapping_max_white, noise_granularity, noise_seed
	).rgb,

	scrolling_color = calculate_lighting(
		face_tbn, scrolling_tangent_space_normal, camera_to_fragment_tangent_space,

		shadow_and_volumetric_light, scrolling_material_properties_and_ambient_strength.xyz,
		still_and_scrolling_light_colors[1], scrolling_dir_to_light, scrolling_albedo_color,

		scrolling_material_properties_and_ambient_strength.w,
		tone_mapping_max_white, noise_granularity, noise_seed
	).rgb;

	////////// This is the old way of mixing the colors

	/*
	color = mix(
		scrolling_color * still_color + still_color,
		scrolling_color + still_color,
		texture_transition_weight
	);
	*/

	////////// And this is the new way

	bool still_color_is_black = (still_albedo_color.rgb == vec3(0.0f));

	color = mix(
		still_color_is_black ? scrolling_color : still_color,
		still_color_is_black ? still_color : scrolling_color,
		texture_transition_weight
	);
}
