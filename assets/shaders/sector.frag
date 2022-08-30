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

/* This code was developed from https://learnopengl.com/Advanced-Lighting/Parallax-Mapping.
The LOD system that transitions between the plain and parallax UV was based on section 5.4.3 from
	https://advances.realtimerendering.com/s2006/Chapter5-Parallax_Occlusion_Mapping_for_detailed_surface_rendering.pdf. */
vec3 get_parallax_UV(void) {
	/* TODO:
	- Aliasing
	- Very slow at times
	- Texture scrolling (note: clamping UV to [0.0f, 1.0f] anywhere doesn't fix this)

	- Apply to all world entities
	- Make the parallax parameters part of the uniform block
	- See here: github.com/Rabbid76/graphics-snippets/blob/master/documentation/normal_parallax_relief.md
	*/

	const float
		min_layers = 16.0f, max_layers = 64.0f,
		height_scale = 0.03f, lod_cutoff = 1.5f;

	////////// LOD calculations

	float lod = textureQueryLod(diffuse_sampler, UV.xy).x;

	/* For all LOD values above `lod_cutoff`, the
	plain UV is used, skipping a lot of work */
	if (lod > lod_cutoff) return UV;

	/* For LODs between `lod_cutoff - lod_blend_range` to `lod_cutoff`,
	blending occurs between the parallax and plain UV */
	const float lod_blend_range = (lod_cutoff < 1.0f) ? lod_cutoff : 1.0f;
	const float min_lod_for_blending = lod_cutoff - lod_blend_range;

	/* If full, unblended parallax mapping is used, this will be below 0,
	so the `max` stops the blend weight from being negative */
	float lod_weight = max(lod - min_lod_for_blending, 0.0f);

	////////// Tracing a ray against the diffuse color texture, which is interpreted as a heightmap

	vec3 view_dir = normalize(camera_fragment_delta_tangent_space);

	/* More layers will be rendered if the view direction is steeper. I'm not using the lod to determine
	the number of layers because while steep angles may yield higher mip levels when using anisotropic
	filtering, the mip level also increases for far-away objects, and the number of layers should only
	be dependent on how steep the view angle is. */
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

	float UV_weight = depth_after / (depth_after - depth_before);

	////////// Getting the parallax UV

	vec2 parallax_UV = mix(curr_UV.xy, prev_UV.xy, UV_weight);
	vec2 lod_parallax_UV = mix(parallax_UV, UV.xy, lod_weight);
	return vec3(lod_parallax_UV, UV.z);
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
