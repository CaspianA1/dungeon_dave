#version 400 core

#include "common/normal_utils.frag"
#include "common/UV_utils.frag"

noperspective in float scrolling_UV_x;
noperspective in vec2 UV;

in vec3 fragment_pos_tangent_space; // TODO: make flat

out vec3 color;

uniform float
	texture_transition_weight, specular_exponent,
	scrolling_bilinear_diffuse_percent,
	scrolling_bilinear_normal_percent;

uniform vec3 light_pos_tangent_space;
uniform sampler2D still_diffuse_sampler, scrolling_diffuse_sampler, scrolling_normal_map_sampler;

//////////

// TODO: use the world shading functions instead of this one, if possible
vec3 blinn_phong(const sampler2D diffuse_sampler, const vec2 diffuse_UV,
	const vec3 fragment_normal, const vec3 dir_to_light, const vec3 halfway_dir) {

	float
		diffuse = max(dot(fragment_normal, dir_to_light), 0.0f),
		cos_angle_of_incidence = max(dot(fragment_normal, halfway_dir), 0.0f);

	float specular = pow(cos_angle_of_incidence, specular_exponent);

	return texture(diffuse_sampler, diffuse_UV).rgb * (diffuse + specular);
}

void main(void) {
	const vec3
		camera_pos_tangent_space = vec3(0.0f, 0.0f, 1.0f),
		face_normal_tangent_space = vec3(0.0f, 0.0f, 1.0f);

	//////////

	vec2 scrolling_UV_for_diffuse = vec2(scrolling_UV_x, UV.y);
	vec2 scrolling_UV_for_normal = scrolling_UV_for_diffuse;

	adjust_UV_for_pixel_art_filtering(scrolling_bilinear_diffuse_percent,
		textureSize(scrolling_diffuse_sampler, 0), scrolling_UV_for_diffuse);

	adjust_UV_for_pixel_art_filtering(scrolling_bilinear_normal_percent,
		textureSize(scrolling_normal_map_sampler, 0), scrolling_UV_for_normal);

	//////////

	vec3
		scrolling_fragment_normal = get_tangent_space_normal_2D(scrolling_normal_map_sampler, scrolling_UV_for_normal).xyz,

		dir_to_light = normalize(light_pos_tangent_space - fragment_pos_tangent_space),
		view_dir = normalize(camera_pos_tangent_space - fragment_pos_tangent_space);

	vec3 halfway_dir = normalize(dir_to_light + view_dir);

	//////////

	vec3
		still_base = blinn_phong(still_diffuse_sampler, UV, face_normal_tangent_space, dir_to_light, halfway_dir),
		scroll_base = blinn_phong(scrolling_diffuse_sampler, scrolling_UV_for_diffuse, scrolling_fragment_normal, dir_to_light, halfway_dir);

	color = mix(still_base * scroll_base, still_base + scroll_base, texture_transition_weight);
}
