#version 400 core

#include "num_cascades.geom" // `num_cascades.geom` is written to by the CPU before any shader compilation

// TODO: share `light_view_projection_matrices` with `depth.geom`

in float world_depth_value;

uniform float cascade_plane_distances[NUM_CASCADES - 1];
uniform mat4 light_view_projection_matrices[NUM_CASCADES];
uniform sampler2DArray shadow_cascade_sampler;

float in_csm_shadow(vec3 fragment_pos_world_space) {
	////////// Selecting a cascade

	int layer = -1; // TODO: select the cascade using some constant-time math
	const int num_splits_between_cascades = int(NUM_CASCADES) - 1;

	for (int i = 0; i < num_splits_between_cascades; i++) {
		if (cascade_plane_distances[i] > world_depth_value) {
			layer = i;
			break;
		}
	}

	layer = (layer == -1) ? num_splits_between_cascades : layer;

	////////// Testing to see if the cascade's fragment is in shadow

	vec4 fragment_pos_light_space = light_view_projection_matrices[layer] * vec4(fragment_pos_world_space, 1.0f);
	vec3 UV = fragment_pos_light_space.xyz * 0.5f + 0.5f;

	float occluder_depth = texture(shadow_cascade_sampler, vec3(UV.xy, layer)).r;
	return float(UV.z <= occluder_depth);
}
