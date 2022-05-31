#version 330 core

// TODO: initialize all of these uniforms

uniform int num_cascades;
uniform float cascade_plane_distances[3]; // TODO: find out if the length of this array should equal `num_cascades`
uniform mat4 view; // Is this the light view matrix?
uniform sampler2DArray cascaded_shadow_map;

uint get_cascade_layer(vec3 fragment_pos_world_space) {
	vec4 fragment_pos_world_space_4D = vec4(fragment_pos_world_space, 1.0f);

	vec4 fragment_pos_view_space = view * fragment_pos_world_space_4D;
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

	float occluder_depth = texture(cascaded_shadow_map, vec3(fragment_pos_light_space.xy, layer)).r; 
	return occluder_depth + 0.005f > fragment_pos_light_space.z;
}
