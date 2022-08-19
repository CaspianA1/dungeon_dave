#version 400 core

#include "common/world_shading.frag"
#include "common/normal_utils.frag"

flat in uint face_id;
in float world_depth_value;

out vec3 color;

vec3 get_face_fragment_normal(const vec3 UV) {
	vec3 ts = get_tangent_space_normal_3D(normal_map_sampler, UV); // `ts` = tangent space

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
	vec3 fragment_normal = get_face_fragment_normal(UV);

	color = calculate_light(world_depth_value, UV, fragment_normal).rgb;
	color = postprocess_light(UV.xy, color);
}
