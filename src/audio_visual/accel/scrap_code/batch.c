// A 'batch' is a collection of objects to be rendered in one draw call.

/* Same batch for different uses, so can re-write sizes and type enum
In drawing, draw non-height-zero sectors and height-zero sectors in their own calls */

// Starting with a SectorBatch

typedef struct {
	GLsizei
		curr_num_drawable,
		max_num_drawable, triangles_per_primitive,
		bytes_per_primitive, bytes_per_vertex;

	GLenum type_enum;
	GLuint vbo;
} SectorBatch;

// bytes_per_mesh_type param can be different for height-zero flat planes and normal sectors
SectorBatch init_sector_batch(const GLsizei max_num_drawable, const GLsizei triangles_per_primitive,
	const GLsizei bytes_per_primitive, const GLsizei bytes_per_vertex, const GLenum type_enum) {

	SectorBatch sector_batch = {
		.curr_num_drawable = 0,
		.max_num_drawable = max_num_drawable,
		.triangles_per_primitive = triangles_per_primitive,
		.bytes_per_primitive = bytes_per_primitive,
		.bytes_per_vertex = bytes_per_vertex,
		.type_enum = type_enum
	};

	glGenBuffers(1, &sector_batch.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, sector_batch.vbo);
	glBufferData(GL_ARRAY_BUFFER, max_num_drawable * bytes_per_primitive, NULL, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(0, 3, type_enum, GL_FALSE, bytes_per_vertex, NULL);
	glVertexAttribPointer(1, 2, type_enum, GL_FALSE, bytes_per_vertex, (void*) 3);

	return sector_batch;
}

void deinit_sector_batch(const SectorBatch* const sector_batch) {
	glDeleteBuffers(1, &sector_batch -> vbo);
}

void prepare_for_sector_batching(SectorBatch* const sector_batch) {
	sector_batch -> curr_num_drawable = 0;
	glBindBuffer(GL_ARRAY_BUFFER, sector_batch -> vbo);
}

// Works assuming that sector batch vbo is already bound
void add_to_sector_batch(SectorBatch* const sector_batch, const void* const data) {
	const GLsizeiptr batch_entry_offset =
		sector_batch -> curr_num_drawable++ * sector_batch -> bytes_per_primitive;

	glBufferSubData(GL_ARRAY_BUFFER, batch_entry_offset, sector_batch -> bytes_per_primitive, data);
}

void draw_sector_batch(SectorBatch* const sector_batch) {
	glDrawArrays(GL_TRIANGLES, 0, 3 * sector_batch -> triangles_per_primitive * sector_batch -> curr_num_drawable);
	sector_batch -> curr_num_drawable = 0;
}

void draw_unculled_sectors_via_batch(Camera* const camera, SectorBatch* const batch, const SectorList sector_list) {
	vec4 frustum_planes[6];
	glm_frustum_planes(camera -> model_view_projection, frustum_planes);

	for (int i = 0; i < sector_list.length; i++) {
		const Sector sector = sector_list.data[i];
		const SectorArea area = sector.area;

		// In format of bottom left and top right
		vec3 aabb_corners[2] = {{area.origin[0], 0.0f, area.origin[1]}};
		glm_vec3_add(aabb_corners[0], (vec3) {area.size[0], area.height, area.size[1]}, aabb_corners[1]);

		// If sector in view
		if (glm_aabb_frustum(aabb_corners, frustum_planes)) {
			printf("Draw #%d\n", i + 1);
			add_to_sector_batch(batch, NULL); // Feed sector vertices later on; store in sector, instead of vbo
		}
	}

	draw_sector_batch(batch);
}
