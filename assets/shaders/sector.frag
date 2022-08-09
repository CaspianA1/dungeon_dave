#version 400 core

#include "common/world_shading.frag"
#include "common/normal_utils.frag"

flat in uint face_id;
in float world_depth_value;

out vec3 color;

uniform vec2 UV_translation;
uniform vec3 UV_translation_area[2];

/* Each level may have an area where sector UV
coordinates are re-translated for artistic purposes */
vec2 retranslate_UV(const vec2 untranslated_UV) {
	bvec3
		in_top_left_extent = bvec3(step(UV_translation_area[0], fragment_pos_world_space)),
		in_bottom_right_extent = bvec3(step(fragment_pos_world_space, UV_translation_area[1]));

	bool in_translation_area = in_top_left_extent == in_bottom_right_extent == true;

	return UV_translation * float(in_translation_area) + untranslated_UV;
}

vec3 get_face_fragment_normal(const vec3 UV) {
	vec3 ts = get_tangent_space_normal(UV); // `ts` = tangent space

	// No matrix multiplication here! :)
	vec3 rotated_vectors[5] = vec3[5](
		vec3(ts.xz, -ts.y), // Flat
		vec3(ts.zy, -ts.x), // Right
		ts, // Bottom (equal to tangent space)
		vec3(-ts.z, ts.yx), // Left
		vec3(-ts.x, ts.y, -ts.z) // Top (opposite of tangent space)
	);

	return rotated_vectors[face_id];
}

void main(void) {
	vec3 translated_UV = vec3(retranslate_UV(UV.xy), UV.z);

	vec3 fragment_normal = get_face_fragment_normal(translated_UV);

	color = calculate_light(world_depth_value, translated_UV, fragment_normal).rgb;
	color = postprocess_light(translated_UV.xy, color);
}
