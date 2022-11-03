#version 400 core

#include "common/world_shading.vert"
#include "common/billboard_transform.vert"

void main(void) {
	mat3 tbn = billboard_front_facing_tbn;
	bool is_on_frontside = dot(tbn[2], billboard_center_world_space - camera_pos_world_space) < 0.0f;
	tbn[2] *= float(is_on_frontside) * 2.0f - 1.0f; // Multiplying the normal by -1 if needed

	set_common_outputs(get_billboard_vertex(-tbn[0]), tbn);

	UV = vec3(get_quad_UV(), texture_id);
}
