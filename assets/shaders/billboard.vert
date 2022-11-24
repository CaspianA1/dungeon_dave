#version 400 core

#include "common/world_shading.vert"
#include "common/billboard_transform.vert"

void main(void) {
	material_index = billboard_material_index;
	bilinear_percents_index = 1u;

	UV = vec3(get_quad_UV(), billboard_texture_id);

	//////////

	mat3 tbn = billboard_front_facing_tbn;
	float side_sign = sign(dot(tbn[2], camera_pos_world_space - billboard_center_world_space));
	tbn[2] *= side_sign; // Flipping the normal if needed

	set_common_outputs(get_billboard_vertex(-tbn[0]), tbn);
}
