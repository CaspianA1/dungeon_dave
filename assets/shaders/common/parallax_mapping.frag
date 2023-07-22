#version 400 core

in vec3 camera_to_fragment_tangent_space;

/* This code was developed from https://learnopengl.com/Advanced-Lighting/Parallax-Mapping.
The LOD system that transitions between the plain and parallax UV was based on section 5.4.3 from
https://advances.realtimerendering.com/s2006/Chapter5-Parallax_Occlusion_Mapping_for_detailed_surface_rendering.pdf. */
vec3 get_parallax_UV(const vec3 UV, const sampler2DArray normal_sampler) {
	/* TODO:
	- Aliasing
	- Very slow at times
	- Texture scrolling (note: clamping UV to [0.0f, 1.0f] anywhere doesn't fix this); perhaps try TexNonRepeating + discard?
	- Note: scrolling called texture swimming here: https://casual-effects.com/research/McGuire2005Parallax/index.html
	- A possible swimming fix: https://gamedev.net/forums/topic/664276-solved-detail-mapping-parallax-texture-swimming/5201186/
	- Skip zero-alpha areas as an optimization, without weird alpha stitch problems? Check for zero alpha + a zero fwidth of alpha?
	- Some weapon sprite parallax looks a bit odd when out of bounds; e.g. the snazzy shotgun or the reload pistol
	- When doing LOD, decrease the height scale too for a smoother transition (after the cutoff, return the normal UV,
		otherwise keep or scale the height scale)

	- Apply to the title screen
	- Make the parallax parameters part of the uniform block
	- See here: github.com/Rabbid76/graphics-snippets/blob/master/documentation/normal_parallax_relief.md
	- Interval mapping instead? https://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.87.5935&rep=rep1&type=pdf
	- Allow parallax mapping to be enabled or disabled (this will be a user setting eventually)
	*/

	if (!parallax_mapping.enabled) return UV;

	////////// LOD calculations

	float lod = textureQueryLod(normal_sampler, UV.xy).x;

	/* For all LOD values above `lod_cutoff`, the plain UV is used, skipping a lot
	of work. Anything below the cutoff gets a progressively smaller height scale. */
	if (lod > parallax_mapping.lod_cutoff) return UV;

	float lod_percent = min(lod / parallax_mapping.lod_cutoff, 1.0f);
	float lod_height_scale = parallax_mapping.height_scale * (1.0f - lod_percent);

	////////// Tracing a ray against the alpha channel of the normal map, which is an inverted heightmap.

	vec3 view_dir = normalize(camera_to_fragment_tangent_space);

	/* More layers will be rendered if the view direction is steeper. I'm not using the lod to determine
	the number of layers because while steep angles may yield higher mip levels when using anisotropic
	filtering, the mip level also increases for far-away objects, and the number of layers should only
	be dependent on how steep the view angle is.

	TODO: would snapping the number of layers to some regularly repeated integer, like
	4, 8, 12, ... decrease fragment shader divergence and increase performance? Not sure. */
	float num_layers = mix(parallax_mapping.max_layers, parallax_mapping.min_layers, max(view_dir.z, 0.0f));

	#define PARALLAX_SAMPLE(UV) texture(normal_sampler, UV).a

	float
		layer_depth = 1.0f / num_layers, curr_layer_depth = 0.0f,
		curr_depth_map_value = PARALLAX_SAMPLE(UV);

	vec2 delta_UV = (view_dir.xy / view_dir.z) * (lod_height_scale / num_layers);
	vec3 curr_UV = UV;

	while (curr_layer_depth < curr_depth_map_value) {
		curr_UV.xy += delta_UV;
		curr_depth_map_value = PARALLAX_SAMPLE(curr_UV);
		curr_layer_depth += layer_depth;
	}

	vec3 prev_UV = vec3(curr_UV.xy - delta_UV, UV.z);

	float
		depth_before = PARALLAX_SAMPLE(prev_UV) - curr_layer_depth + layer_depth,
		depth_after = curr_depth_map_value - curr_layer_depth;

	float UV_weight = clamp(depth_after / (depth_after - depth_before), 0.0f, 1.0f);

	////////// Getting the parallax UV

	return vec3(mix(curr_UV.xy, prev_UV.xy, UV_weight), UV.z);

	#undef PARALLAX_SAMPLE
}
