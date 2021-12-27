#ifndef SECTOR_C
#define SECTOR_C

#define inlinable static inline
#define wmalloc malloc
#define wfree free

#define bit_is_set(bits, mask) ((bits) & (mask))
#define set_bit(bits, mask) ((bits) |= (mask))

typedef struct {
	int chunk_dimensions[2];
	size_t alloc_bytes;
	byte* data;
} StateMap;

#include "../../../main/statemap.c"
#include "headers/sector.h"
#include "headers/texture.h"
#include "face.c"
#include "batch_draw_context.c"
#include "list.c"

byte* map_point(byte* const map, const byte x, const byte y, const byte map_width) {
	return map + (y * map_width + x);
}

// Attributes here = height and texture id
static byte point_matches_sector_attributes(const Sector* const sector_ref,
	const byte* const heightmap, const byte* const texture_id_map,
	const byte x, const byte y, const byte map_width) {

	return
		*map_point((byte*) heightmap, x, y, map_width) == sector_ref -> visible_heights.max
		&& *map_point((byte*) texture_id_map, x, y, map_width) == sector_ref -> texture_id;
}

// Gets length across, and then adds to area size y until out of map or length across not eq
static Sector form_sector_area(Sector sector, const StateMap traversed_points,
	const byte* const heightmap, const byte* const texture_id_map,
	const byte map_width, const byte map_height) {

	byte top_right_corner_x = sector.origin[0];
	const byte origin_y = sector.origin[1];

	while (top_right_corner_x < map_width
		&& !get_statemap_bit(traversed_points, top_right_corner_x, origin_y)
		&& point_matches_sector_attributes(&sector, heightmap, texture_id_map, top_right_corner_x, origin_y, map_width)) {

		sector.size[0]++;
		top_right_corner_x++;
	}

	// Now, area.size[0] equals the first horizontal length of equivalent height found
	for (byte y = origin_y; y < map_height; y++, sector.size[1]++) {
		for (byte x = sector.origin[0]; x < top_right_corner_x; x++) {
			if (!point_matches_sector_attributes(&sector, heightmap, texture_id_map, x, y, map_width))
				goto clear_map_area;
		}
	}

	clear_map_area:

	for (byte y = origin_y; y < origin_y + sector.size[1]; y++) {
		for (byte x = sector.origin[0]; x < sector.origin[0] + sector.size[0]; x++)
			set_statemap_bit(traversed_points, x, y);
	}

	return sector;
}

List generate_sectors_from_maps(
	const byte* const heightmap, const byte* const texture_id_map,
	const byte map_width, const byte map_height) {

	// >> 3 = / 8. Works pretty well for my maps.
	const size_t sector_amount_guess = map_width * map_height >> 3;
	List sectors = init_list(sector_amount_guess, Sector);

	/* StateMap used instead of copy of heightmap with null map points, b/c 1. less bytes used
	and 2. for forming faces, will need original heightmap to be unmodified */
	const StateMap traversed_points = init_statemap(map_width, map_height);

	for (byte y = 0; y < map_height; y++) {
		for (byte x = 0; x < map_width; x++) {
			if (get_statemap_bit(traversed_points, x, y)) continue;

			const byte
				height = *map_point((byte*) heightmap, x, y, map_width),
				texture_id = *map_point((byte*) texture_id_map, x, y, map_width);

			if (texture_id >= MAX_NUM_TEXTURES) {
				fprintf(stderr, "Sector creation failure at pos {%d, %d}; texture ID = %d.\n", x, y, texture_id);
				fail("create a sector because the texture ID is too large", TextureIDIsTooLarge);
			}

			const Sector sector = form_sector_area((Sector) {
				.texture_id = texture_id,
				.origin = {x, y}, .size = {0, 0},
				.visible_heights = {.min = 0, .max = height}
			}, traversed_points, heightmap, texture_id_map, map_width, map_height);

			push_ptr_to_list(&sectors, &sector);
		}
	}

	deinit_statemap(traversed_points);
	return sectors;
}

////////// This next part concerns the drawing of sectors

void init_sector_draw_context(
	IndexedBatchDrawContext* const draw_context, const byte* const heightmap,
	const byte* const texture_id_map, const byte map_width, const byte map_height) {

	// This is freed by demo_17_deinit; its ownership is moved to the draw context
	List sectors = generate_sectors_from_maps(heightmap, texture_id_map, map_width, map_height);

	// This guess seems to work pretty well on my maps
	const size_t index_list_length_guess = sectors.length * 3;

	List // Index list and face mesh list have the same amount of entries (in terms of elements, not bytes)
		index_list = init_list(index_list_length_guess, buffer_index_t[indices_per_face]), // Also freed in demo_17_deinit
		face_mesh_list = init_list(index_list_length_guess, mesh_component_t[vars_per_face]); // Freed at the end of this function

	for (size_t i = 0; i < sectors.length; i++) {
		Sector* const sector_ref = ((Sector*) sectors.data) + i;
		sector_ref -> ibo_range.start = index_list.length * indices_per_face;

		const Sector sector = *sector_ref;
		const Face flat_face = {Flat, {sector.origin[0], sector.origin[1]}, {sector.size[0], sector.size[1]}};
		add_face_mesh_to_list(flat_face, sector.visible_heights.max, 0, sector.texture_id, &face_mesh_list, &index_list);

		byte biggest_face_height = 0;

		init_vert_faces(sector, &face_mesh_list, &index_list,
			heightmap, map_width, map_height, &biggest_face_height);

		sector_ref -> visible_heights.min = sector.visible_heights.max - biggest_face_height;
		sector_ref -> ibo_range.length = index_list.length * indices_per_face - sector_ref -> ibo_range.start;
	}

	////////// This part initializes the sector shader and gpu-side buffers
	draw_context -> c.shader = init_shader_program(sector_vertex_shader, sector_fragment_shader);

	draw_context -> c.object_buffers.cpu = sectors;
	draw_context -> index_buffers.cpu = index_list;

	const GLsizeiptr
		total_vertex_bytes = face_mesh_list.length * vars_per_face * sizeof(mesh_component_t),
		total_index_bytes = face_mesh_list.length * sizeof(buffer_index_t[indices_per_face]);

	GLuint vbo_and_ibo[2];
	glGenBuffers(2, vbo_and_ibo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_and_ibo[0]);
	glBufferData(GL_ARRAY_BUFFER, total_vertex_bytes, face_mesh_list.data, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_and_ibo[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, total_index_bytes, NULL, GL_DYNAMIC_DRAW);

	draw_context -> c.object_buffers.gpu = vbo_and_ibo[0];
	draw_context -> index_buffers.gpu = vbo_and_ibo[1];
	draw_context -> c.gpu_buffer_ptr = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);

	deinit_list(face_mesh_list);
}

static byte sector_in_view_frustum(const Sector sector, vec4 frustum_planes[6]) {
	// First corner is bottom left (if looking top-down, top left), and second is top right
	vec3 aabb_corners[2] = {{sector.origin[0], sector.visible_heights.min, sector.origin[1]}};

	aabb_corners[1][0] = aabb_corners[0][0] + sector.size[0];
	aabb_corners[1][1] = aabb_corners[0][1] + sector.visible_heights.max - sector.visible_heights.min;
	aabb_corners[1][2] = aabb_corners[0][2] + sector.size[1];

	return glm_aabb_frustum(aabb_corners, frustum_planes);
}

static void draw_sectors(const IndexedBatchDrawContext* const indexed_draw_context,
	const Camera* const camera, const buffer_index_t num_visible_indices) {

	const BatchDrawContext* const draw_context = &indexed_draw_context -> c;

	const GLuint sector_shader = draw_context -> shader;
	glUseProgram(sector_shader);
	use_texture(draw_context -> texture_set, sector_shader, TexSet);

	static byte first_call = 1;
	static GLint
		model_view_projection_id, ambient_strength_id,
		diffuse_strength_id, light_pos_id;

	if (first_call) {
		model_view_projection_id = glGetUniformLocation(sector_shader, "model_view_projection");
		ambient_strength_id = glGetUniformLocation(sector_shader, "ambient_strength");
		diffuse_strength_id = glGetUniformLocation(sector_shader, "diffuse_strength");
		light_pos_id = glGetUniformLocation(sector_shader, "light_pos_world_space");
		first_call = 0;
	}

	glUniform1f(ambient_strength_id, 0.4f);
	glUniform1f(diffuse_strength_id, 0.6f);
	glUniform3fv(light_pos_id, 1, camera -> pos);
	glUniformMatrix4fv(model_view_projection_id, 1, GL_FALSE, &camera -> model_view_projection[0][0]);

	glBindBuffer(GL_ARRAY_BUFFER, draw_context -> object_buffers.gpu);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(0, 3, MESH_COMPONENT_TYPENAME, GL_FALSE, bytes_per_vertex, NULL);
	glVertexAttribIPointer(1, 1, MESH_COMPONENT_TYPENAME, bytes_per_vertex, (void*) (3 * sizeof(mesh_component_t)));

	glDrawElements(GL_TRIANGLES, num_visible_indices, BUFFER_INDEX_TYPENAME, NULL);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}

void draw_visible_sectors(const IndexedBatchDrawContext* const indexed_draw_context, const Camera* const camera) {
	/* Each vec4 plane is composed of a vec3 surface normal and
	the closest distance to the origin in the fourth component */

	const BatchDrawContext* const draw_context = &indexed_draw_context -> c;
	static vec4 frustum_planes[6];
	glm_frustum_planes((vec4*) camera -> view_projection, frustum_planes);

	const List sectors = draw_context -> object_buffers.cpu;

	const buffer_index_t* const indices = indexed_draw_context -> index_buffers.cpu.data;
	buffer_index_t* const ibo_ptr = draw_context -> gpu_buffer_ptr, num_visible_indices = 0;

	for (size_t i = 0; i < sectors.length; i++) {
		const Sector* sector = ((Sector*) sectors.data) + i;

		buffer_index_t num_indices = 0;
		const buffer_index_t ibo_start_index = sector -> ibo_range.start;

		while (i < sectors.length && sector_in_view_frustum(*sector, frustum_planes)) {
			num_indices += sector++ -> ibo_range.length;
			i++;
		}

		if (num_indices != 0) {
			memcpy(ibo_ptr + num_visible_indices, indices + ibo_start_index, num_indices * sizeof(buffer_index_t));
			num_visible_indices += num_indices;
		}
	}

	/* (triangle counts, 12 vs 17):
	palace: 1466 vs 1130. tpt: 232 vs 150.
	pyramid: 816 vs 542. maze: 5796 vs 6114.
	terrain: 150620 vs 86588. */

	// If looking out at the distance with no sectors, why call glDrawElements, or do any state switching, at all?
	if (num_visible_indices != 0) draw_sectors(indexed_draw_context, camera, num_visible_indices);
}

#endif
