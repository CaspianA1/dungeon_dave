#version 400 core

#include "csm.common"

// TODO: initialize all of these uniforms

uniform float cascade_plane_distances[NUM_CASCADE_LAYERS];
//  Note: `light_space_matrices` is implicitly shared with `depth.geom` (same shader). TODO: is `view` in light view, or world view?
uniform mat4 view, light_space_matrices[NUM_CASCADE_LAYERS];
uniform sampler2DArray cascade_sampler;

bool in_shadow(vec3 fragment_pos_world_space) {
	vec4 fragment_pos_world_space_4D = vec4(fragment_pos_world_space, 1.0f);

	////////// Selecting a cascade

	vec4 fragment_pos_view_space = view * fragment_pos_world_space_4D;
	float depth_value = abs(fragment_pos_view_space.z);

	int layer = -1; // TODO: select the cascade using some constant-time math
	for (int i = 0; i < NUM_CASCADE_LAYERS; i++) {
		if (depth_value < cascade_plane_distances[i]) {
			layer = i;
			break;
		}
	}

	layer = (layer == -1) ? int(NUM_CASCADE_LAYERS - 1u) : layer;

	////////// Testing to see if the cascade's fragment is in shadow

	// No slope-scale depth bias here because ESM will be used later. TODO: use ESM.

	vec4 fragment_pos_light_space = light_space_matrices[layer] * fragment_pos_world_space_4D;
	float occluder_depth = texture(cascade_sampler, vec3(fragment_pos_light_space.xy, layer)).r;
	return occluder_depth + 0.005f > fragment_pos_light_space.z;
}