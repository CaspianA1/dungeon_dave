#ifndef CULLING_C
#define CULLING_C

#include "headers/culling.h"

#include "sector.c"
#include "camera.c"

/* This code culls the sectors outside of the view frustum
And puts the indices of the visible ones into a batch
to be rendered in one draw call via glDrawElements. */

byte sector_in_view_frustum(const Sector sector, vec4 frustum_planes[6]) {
	// Top left and bottom right
	vec3 aabb_corners[2] = {{sector.origin[0], 0.0f, sector.origin[1]}};

	glm_vec3_add(
		aabb_corners[0],
		(vec3) {sector.size[0], sector.height, sector.size[1]},
		aabb_corners[1]);
	
	return glm_aabb_frustum(aabb_corners, frustum_planes);
}

void draw_sectors_in_view_frustum(const SectorList* const sector_list, const Camera* const camera) {
	vec4 frustum_planes[6];
	glm_frustum_planes((vec4*) camera -> model_view_projection, frustum_planes);

	const List sectors = sector_list -> sectors;

	const index_type_t* const indices = sector_list -> indices.data;
	index_type_t* const ibo_ptr = sector_list -> ibo_ptr, num_visible_indices = 0;

	for (size_t i = 0; i < sectors.length; i++) {
		const Sector* sector = ((Sector*) sectors.data) + i;

		index_type_t num_indices = 0;
		const index_type_t start_index_index = sector -> ibo_range.start;

		while (i < sectors.length && sector_in_view_frustum(*sector, frustum_planes)) {
			num_indices += sector++ -> ibo_range.length;
			i++;
		}

		if (num_indices != 0) {
			memcpy(ibo_ptr + num_visible_indices, indices + start_index_index, num_indices * sizeof(index_type_t));
			num_visible_indices += num_indices;
		}
	}

	glDrawElements(GL_TRIANGLES, num_visible_indices, INDEX_TYPE_ENUM, NULL);
}

#endif