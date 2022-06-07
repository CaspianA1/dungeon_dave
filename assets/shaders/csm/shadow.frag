#version 400 core

#include "num_cascades.geom" // `num_cascades.geom` is written to by the CPU before any shader compilation

// TODO: share `light_view_projection_matrices` with `depth.geom`

uniform float cascade_plane_distances[NUM_CASCADES - 1];
uniform mat4 light_view_projection_matrices[NUM_CASCADES];
uniform sampler2DArrayShadow shadow_cascade_sampler;

float get_csm_shadow_from_layer(uint layer, vec3 fragment_pos_world_space) {
	vec4 fragment_pos_light_space = light_view_projection_matrices[layer] * vec4(fragment_pos_world_space, 1.0f);
	vec3 UV = fragment_pos_light_space.xyz * 0.5f + 0.5f;

	return texture(shadow_cascade_sampler, vec4(UV.xy, layer, UV.z));
}

float in_csm_shadow(float world_depth_value, vec3 fragment_pos_world_space) {
	const int num_splits_between_cascades = int(NUM_CASCADES) - 1;
	int layer = num_splits_between_cascades; // TODO: select the cascade using some constant-time math

	for (int i = 0; i < num_splits_between_cascades; i++) {
		if (cascade_plane_distances[i] > world_depth_value) {
			layer = i;
			break;
		}
	}

	return get_csm_shadow_from_layer(uint(layer), fragment_pos_world_space);
}
