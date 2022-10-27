#version 400 core

#include "common/world_shading.vert"
#include "common/billboard_transform.vert"

void main(void) {
	vec3 front_facing_normal = billboard_front_facing_tbn[2];
	bool is_on_backside = dot(front_facing_normal, billboard_center_world_space - camera_pos_world_space) > 0.0f;

	mat3 tbn = billboard_front_facing_tbn; // Flipping the normal if needed
	tbn[2] = is_on_backside ? -front_facing_normal : front_facing_normal;

	set_common_outputs(get_billboard_vertex(-tbn[0]), tbn);

	UV = vec3(get_quad_UV(), texture_id);
}
