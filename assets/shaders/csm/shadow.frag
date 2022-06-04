#version 400 core

#include "num_cascades.geom" // `num_cascades.frag` is written to by the CPU before any shader compilation

// TODO: initialize all of these uniforms, and share `light_space_matrices` with `depth.geom`

uniform float cascade_plane_distances[NUM_CASCADES];
uniform mat4 camera_view, light_space_matrices[NUM_CASCADES];
uniform sampler2DArray cascade_sampler;

bool in_shadow(vec3 fragment_pos_world_space) {
	vec4 fragment_pos_world_space_4D = vec4(fragment_pos_world_space, 1.0f);

	////////// Selecting a cascade

	vec4 fragment_pos_view_space = camera_view * fragment_pos_world_space_4D;
	float depth_value = abs(fragment_pos_view_space.z); // TODO: see if I can just do a simple negation instead

	int layer = -1; // TODO: select the cascade using some constant-time math
	for (int i = 0; i < NUM_CASCADES; i++) {
		if (depth_value < cascade_plane_distances[i]) {
			layer = i;
			break;
		}
	}

	layer = (layer == -1) ? int(NUM_CASCADES - 1u) : layer;

	////////// Testing to see if the cascade's fragment is in shadow

	// No slope-scale depth bias here because ESM will be used later. TODO: use ESM.

	vec4 fragment_pos_light_space = light_space_matrices[layer] * fragment_pos_world_space_4D;
	vec3 cascade_UV = fragment_pos_light_space.xyz * 0.5f + 0.5f; // TODO: incorporate a bias matrix on the CPU

	float occluder_depth = texture(cascade_sampler, vec3(cascade_UV.xy, layer)).r;
	return occluder_depth + 0.005f > cascade_UV.z;
}
