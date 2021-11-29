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

	// ibo already bound
	GLuint* const ibo_ptr = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY); // TODO: if possible, map once and store in sector list
	(void) ibo_ptr;

	for (size_t i = 0; i < sectors.length; i++) {
		const Sector sector = ((Sector*) sectors.data)[i];
		if (sector_in_view_frustum(sector, frustum_planes)) {
			// Add to ibo buffer
			// printf("%d -> %d\n", sector.ibo_range.start, sector.ibo_range.length);
			// printf("Height %d visible\n", sector.height);
		}
	}
	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
}

#endif
