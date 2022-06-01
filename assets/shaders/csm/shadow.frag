#version 400 core


#include "csm.common"

// TODO: initialize all of these uniforms

uniform float cascade_plane_distances[NUM_CASCADE_LAYERS];
// TODO: is `view` the light view matrix, or the world view marix? Also, share `light_space_matrices` with `depth.geom`
uniform mat4 view, light_space_matrices[NUM_CASCADE_LAYERS]; // TODO: use `light_space_matrices`
uniform sampler2DArray cascade_sampler;

uint get_cascade_layer(vec3 fragment_pos_world_space) {
	vec4 fragment_pos_view_space = view * vec4(fragment_pos_world_space, 1.0f);
	float depth_value = abs(fragment_pos_view_space.z);

	int layer = -1; // TODO: select the cascade using some constant-time math
	for (int i = 0; i < num_cascades; i++) {
		if (depth_value < cascade_plane_distances[i]) {
			layer = i;
			break;
		}
	}

	return uint((layer == -1) ? num_cascades : layer);
}

bool in_shadow(uint layer, vec3 fragment_pos_light_space) {
	// No slope-scale depth bias here because ESM will be used later. TODO: use ESM

	float occluder_depth = texture(cascade_sampler, vec3(fragment_pos_light_space.xy, layer)).r;
	return occluder_depth + 0.005f > fragment_pos_light_space.z;
}
