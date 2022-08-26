#version 400 core

#include "common/world_shading.frag"
#include "common/normal_utils.frag"

flat in uint face_id;
in float world_depth_value;
in vec3 camera_fragment_delta_tangent_space;

out vec3 color;

float sample_heightmap(const vec3 UV) {
	const float neg_one_third = -1.0f / 3.0f;

	vec3 diffuse = texture(diffuse_sampler, UV).rgb;
	return (diffuse.r + diffuse.g + diffuse.b) * neg_one_third + 1.0f;
}

vec3 get_parallax_UV(void) {
	/* TODO:
	- Aliasing
	- Very slow at times (fix this + aliasing through relief mapping?)
	- Texture scrolling
	- Apply to all world entities
	- Make the parallax parameters part of the uniform block
	- The lod blending looks a bit rough
	- See here: github.com/Rabbid76/graphics-snippets/blob/master/documentation/normal_parallax_relief.md
	*/

	const float
		min_layers = 32.0f, max_layers = 64.0f,
		height_scale = 0.03f, lod_threshold = 0.2f;

	float lod = textureQueryLod(diffuse_sampler, UV.xy).y;
	if (lod > lod_threshold) return UV;

	//////////

	vec3 view_dir = normalize(camera_fragment_delta_tangent_space);
	float num_layers = mix(max_layers, min_layers, max(view_dir.z, 0.0f));

	float
		layer_depth = 1.0f / num_layers, curr_layer_depth = 0.0f,
		curr_depth_map_value = sample_heightmap(UV);

	vec2 delta_UV = (view_dir.xy / view_dir.z) * (height_scale / num_layers);
	vec3 curr_UV = UV;

	while (curr_layer_depth < curr_depth_map_value) {
		curr_UV.xy -= delta_UV;
		curr_depth_map_value = sample_heightmap(curr_UV);
		curr_layer_depth += layer_depth;
	}

	vec3 prev_UV = vec3(curr_UV.xy + delta_UV, UV.z);

	float
		depth_before = sample_heightmap(prev_UV) - curr_layer_depth + layer_depth,
		depth_after = curr_depth_map_value - curr_layer_depth;

	float weight = depth_after / (depth_after - depth_before);

	vec3 parallax_UV = vec3(mix(curr_UV.xy, prev_UV.xy, weight), UV.z);

	// Blending between the parallax UV and non-parallax UV for a smoother lod transition
	return mix(parallax_UV, UV, lod / lod_threshold);
}

vec3 get_face_fragment_normal(const vec3 UV) { // TODO: use the tbn instead?
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
	vec3 parallax_UV = get_parallax_UV();
	vec3 fragment_normal = get_face_fragment_normal(parallax_UV);

	color = calculate_light(world_depth_value, parallax_UV, fragment_normal).rgb;
	color = postprocess_light(parallax_UV.xy, color);
}
