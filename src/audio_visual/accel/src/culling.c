#ifndef CULLING_C
#define CULLING_C

#include "headers/culling.h"

#include "sector.c"
#include "camera.c"

/* This code culls the sectors outside of the view frustum
And puts the indices of the visible ones into a batch
to be rendered in one draw call via glDrawElements. */

static byte sector_in_view_frustum(const Sector sector, vec4 frustum_planes[6]) {
	// Bottom left, size
	vec3 aabb_corners[2] = {{sector.origin[0], sector.visible_heights.min, sector.origin[1]}};

	glm_vec3_add(
		aabb_corners[0],
		(vec3) {sector.size[0], sector.visible_heights.max - sector.visible_heights.min, sector.size[1]},
		aabb_corners[1]);

	return glm_aabb_frustum(aabb_corners, frustum_planes);
}

static void draw_sectors_in_view_frustum(const SectorList* const sector_list, const Camera* const camera) {
	static vec4 frustum_planes[6];
	glm_frustum_planes((vec4*) camera -> view_projection, frustum_planes);

	const List sectors = sector_list -> sectors;

	const buffer_index_t* const indices = sector_list -> indices.data;
	buffer_index_t* const ibo_ptr = sector_list -> ibo_ptr, num_visible_indices = 0;

	for (size_t i = 0; i < sectors.length; i++) {
		const Sector* sector = ((Sector*) sectors.data) + i;

		buffer_index_t num_indices = 0;
		const buffer_index_t start_index_index = sector -> ibo_range.start;

		while (i < sectors.length && sector_in_view_frustum(*sector, frustum_planes)) {
			num_indices += sector++ -> ibo_range.length;
			i++;
		}

		if (num_indices != 0) {
			memcpy(ibo_ptr + num_visible_indices, indices + start_index_index, num_indices * sizeof(buffer_index_t));
			num_visible_indices += num_indices;
		}
	}

	/* (triangle counts, 12 vs 17):
	palace: 1466 vs 1130. tpt: 232 vs 150.
	pyramid: 816 vs 542. maze: 5796 vs 6114.
	terrain: 150620 vs 86588. */
	glDrawElements(GL_TRIANGLES, num_visible_indices, INDEX_TYPE_ENUM, NULL);
}

void draw_sectors(const SectorList* const sector_list, const Camera* const camera, const GLuint sector_shader) {
	glUseProgram(sector_shader);
	use_texture(sector_list -> texture_set, sector_shader, TexSet);

	static byte first_call = 1;
	static GLint model_view_projection_id;

	if (first_call) {
		model_view_projection_id = glGetUniformLocation(sector_shader, "model_view_projection");
		first_call = 0;
	}

	glUniformMatrix4fv(model_view_projection_id, 1, GL_FALSE, &camera -> model_view_projection[0][0]);

	glBindBuffer(GL_ARRAY_BUFFER, sector_list -> vbo);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(0, 3, MESH_TYPE_ENUM, GL_FALSE, bytes_per_vertex, NULL);
	glVertexAttribIPointer(1, 1, MESH_TYPE_ENUM, bytes_per_vertex, (void*) (3 * sizeof(mesh_component_t)));

	// bind_sector_list_vbo_to_vao(sector_list);
	draw_sectors_in_view_frustum(sector_list, camera);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}

#endif
