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

	(void) sector_list;
	const List underlying_sector_list = sector_list -> list;

	for (size_t i = 0; i < underlying_sector_list.length; i++) {
		const Sector sector = ((Sector*) underlying_sector_list.data)[i];
		if (sector_in_view_frustum(sector, frustum_planes)) {
			puts("Visible");
		}
	}
}

#endif
