#version 400 core

#include "common/shared_params.glsl"

in vec3 cube_edge;

out vec3 color;

uniform vec2 cylindrical_cap_blend_widths; // Top, bottom

uniform samplerCube skybox_sampler;

void main(void) {
	vec3 cube_color = texture(skybox_sampler, cube_edge).rgb;

	// https://stackoverflow.com/questions/75466490
	if (cylindrical_cap_blend_widths != vec2(0.0f)) {
		////////// Remapping a cube edge to a cylinder edge

		vec3 cylinder_edge = cube_edge / length(cube_edge.xz);
		cylinder_edge.xz /= max(abs(cylinder_edge.x), abs(cylinder_edge.z));

		const float one_over_atan_of_one = 1.0f / atan(1.0f);
		cylinder_edge.xz = atan(cylinder_edge.xz) * one_over_atan_of_one;

		////////// Blending smoothly into the caps

		// Absolute values of y positions when cap blending starts
		const vec2 abs_y_blend_starts = vec2(1.0f);

		vec2 cap_blend_percents = vec2(cylinder_edge.y, -cylinder_edge.y) + cylindrical_cap_blend_widths - abs_y_blend_starts;
		cap_blend_percents = clamp(cap_blend_percents / cylindrical_cap_blend_widths, 0.0f, 1.0f);

		bool on_cap = (cap_blend_percents != vec2(0.0f));
		float cap_blend_percent = float(on_cap) * max(cap_blend_percents.x, cap_blend_percents.y);

		color = mix(texture(skybox_sampler, cylinder_edge).rgb, cube_color, cap_blend_percent);
	}
	else color = cube_color;

	color *= light_color;
}
