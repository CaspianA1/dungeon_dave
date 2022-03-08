#ifndef SECTOR_C
#define SECTOR_C

#include "headers/sector.h"
#include "data/shaders.c"
#include "face.c"
#include "batch_draw_context.c"
#include "texture.c"
#include "list.c"

//////////

#define inlinable static inline
#define wmalloc malloc
#define wfree free

typedef struct {
	unsigned chunk_dimensions[2];
	buffer_size_t alloc_bytes;
	byte* data;
} StateMap;

#include "../../../main/statemap.c"

//////////

// Attributes here = height and texture id
static byte point_matches_sector_attributes(const Sector* const sector,
	const byte* const heightmap, const byte* const texture_id_map,
	const byte x, const byte y, const byte map_width) {

	return
		*map_point((byte*) heightmap, x, y, map_width) == sector -> visible_heights.max
		&& *map_point((byte*) texture_id_map, x, y, map_width) == sector -> texture_id;
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

List generate_sectors_from_maps(const byte* const heightmap,
	const byte* const texture_id_map, const byte map_size[2]) {

	const byte map_width = map_size[0], map_height = map_size[1];

	// `>> 3` = `/ 8`. Works pretty well for my maps.
	const buffer_size_t sector_amount_guess = (map_width * map_height) >> 3;
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

			if (texture_id >= MAX_NUM_SECTOR_SUBTEXTURES) {
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

void init_sector_draw_context(BatchDrawContext* const draw_context,
	List* const sectors_ref, const byte* const heightmap,
	const byte* const texture_id_map, const byte map_size[2]) {

	List sectors = generate_sectors_from_maps(heightmap, texture_id_map, map_size);

	/* This contains the actual vertex data for faces. `sectors.length * 3` gives a good guess for the
	face/sector ratio. Its ownership, after this function, goes to the draw context (which frees it). */
	List face_meshes = init_list(sectors.length * 3, face_mesh_component_t[components_per_face]);

	for (buffer_size_t i = 0; i < sectors.length; i++) {
		Sector* const sector_ref = ((Sector*) sectors.data) + i;
		sector_ref -> face_range.start = face_meshes.length;

		const Sector sector = *sector_ref;
		const Face flat_face = {Flat, {sector.origin[0], sector.origin[1]}, {sector.size[0], sector.size[1]}};
		add_face_mesh_to_list(flat_face, sector.visible_heights.max, 0, sector.texture_id, &face_meshes);

		byte biggest_face_height = 0;
		init_vert_faces(sector, &face_meshes, heightmap, map_size[0], map_size[1], &biggest_face_height);

		sector_ref -> visible_heights.min = sector.visible_heights.max - biggest_face_height;
		sector_ref -> face_range.length = face_meshes.length - sector_ref -> face_range.start;
	}

	draw_context -> buffers.cpu = face_meshes;
	init_batch_draw_context_gpu_buffer(draw_context, face_meshes.length, bytes_per_face);
	draw_context -> shader = init_shader_program(sector_vertex_shader, sector_fragment_shader);
	*sectors_ref = sectors;
}

static byte sector_in_view_frustum(const Sector sector, const vec4 frustum_planes[6]) {
	// First corner is bottom left (if looking top-down, top left), and second is top right
	vec3 aabb_corners[2] = {{sector.origin[0], sector.visible_heights.min, sector.origin[1]}};

	aabb_corners[1][0] = aabb_corners[0][0] + sector.size[0];
	aabb_corners[1][1] = aabb_corners[0][1] + sector.visible_heights.max - sector.visible_heights.min;
	aabb_corners[1][2] = aabb_corners[0][2] + sector.size[1];

	return glm_aabb_frustum(aabb_corners, (vec4*) frustum_planes);
}

static void draw_sectors(const BatchDrawContext* const draw_context,
	const ShadowMapContext* const shadow_map_context, const Camera* const camera,
	const buffer_size_t num_visible_faces) {

	const GLuint sector_shader = draw_context -> shader;
	glUseProgram(sector_shader);

	static GLint camera_pos_world_space_id, inv_light_dir_id, model_view_projection_id, light_model_view_projection_id;
	static bool first_call = true;

	if (first_call) {
		INIT_UNIFORM(camera_pos_world_space, sector_shader);
		INIT_UNIFORM(inv_light_dir, sector_shader);
		INIT_UNIFORM(model_view_projection, sector_shader);
		INIT_UNIFORM(light_model_view_projection, sector_shader);

		INIT_UNIFORM_VALUE(overall_light_strength, sector_shader, 1f, 1.0f);
		INIT_UNIFORM_VALUE(ambient, sector_shader, 1f, 0.18f); // This also equals the amount of light in shadows
		INIT_UNIFORM_VALUE(shininess, sector_shader, 1f, 16.0f);
		INIT_UNIFORM_VALUE(specular_strength, sector_shader, 1f, 0.6f);
		INIT_UNIFORM_VALUE(min_shadow_variance, sector_shader, 1f, 0.000005f); // 0.000785f
		INIT_UNIFORM_VALUE(light_bleed_reduction_factor, sector_shader, 1f, 0.65f); // 0.2f

		use_texture(draw_context -> texture_set, sector_shader, "texture_sampler", TexSet, SECTOR_TEXTURE_UNIT);
		use_texture(shadow_map_context -> buffer_context.moment_texture,
			sector_shader, "shadow_map_sampler", TexPlain, SHADOW_MAP_TEXTURE_UNIT);

		first_call = false;
	}

	UPDATE_UNIFORM(camera_pos_world_space, 3fv, 1, camera -> pos);
	const GLfloat* const light_dir = shadow_map_context -> light_context.dir;
	INIT_UNIFORM_VALUE(inv_light_dir, sector_shader, 3f, -light_dir[0], -light_dir[1], -light_dir[2]);

	UPDATE_UNIFORM(model_view_projection, Matrix4fv, 1, GL_FALSE, &camera -> model_view_projection[0][0]);
	UPDATE_UNIFORM(light_model_view_projection, Matrix4fv, 1, GL_FALSE,
		&shadow_map_context -> light_context.model_view_projection[0][0]);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(0, 3, MESH_COMPONENT_TYPENAME, GL_FALSE, bytes_per_face_vertex, (void*) 0);
	glVertexAttribIPointer(1, 1, MESH_COMPONENT_TYPENAME, bytes_per_face_vertex, (void*) (3 * sizeof(face_mesh_component_t)));

	glDrawArrays(GL_TRIANGLES, 0, (GLsizei) (num_visible_faces * vertices_per_face));

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}

// Returns the number of visible faces
static buffer_size_t fill_sector_vbo_with_visible_faces(
	const BatchDrawContext* const draw_context,
	const List* const sectors, const Camera* const camera) {

	glBindBuffer(GL_ARRAY_BUFFER, draw_context -> buffers.gpu);

	const Sector* const sector_data = sectors -> data;
	const Sector* const out_of_bounds_sector = sector_data + sectors -> length;

	/* Each vec4 plane in `frustum_planes` is composed of a vec3 surface
	normal and the closest distance to the origin in the fourth component */
	const vec4* const frustum_planes = camera -> frustum_planes;

	const face_mesh_component_t* const face_meshes_cpu = draw_context -> buffers.cpu.data;
	face_mesh_component_t* const face_meshes_gpu = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	buffer_size_t num_visible_faces = 0;

	for (const Sector* sector = sector_data; sector < out_of_bounds_sector; sector++) {
		buffer_size_t num_visible_faces_in_group = 0;
		const buffer_size_t cpu_buffer_start_index = sector -> face_range.start * components_per_face;

		while (sector < out_of_bounds_sector && sector_in_view_frustum(*sector, frustum_planes))
			num_visible_faces_in_group += sector++ -> face_range.length;

		if (num_visible_faces_in_group != 0) {
			memcpy(face_meshes_gpu + num_visible_faces * components_per_face,
				face_meshes_cpu + cpu_buffer_start_index,
				num_visible_faces_in_group * bytes_per_face);

			num_visible_faces += num_visible_faces_in_group;
		}
	}

	/* (triangle counts, 12 vs 17):
	palace: 1466 vs 1130. tpt: 232 vs 150.
	pyramid: 816 vs 542. maze: 5796 vs 6114.
	terrain: 150620 vs 86588. */

	glUnmapBuffer(GL_ARRAY_BUFFER); // If looking out at the distance with no sectors, why do any state switching at all?

	return num_visible_faces;
}

// This is just a utility function
void draw_visible_sectors(const BatchDrawContext* const draw_context,
	const ShadowMapContext* const shadow_map_context, const List* const sectors,
	const Camera* const camera) {

	const buffer_size_t num_visible_faces = fill_sector_vbo_with_visible_faces(draw_context, sectors, camera);
	if (num_visible_faces != 0) draw_sectors(draw_context, shadow_map_context, camera, num_visible_faces);
}

#endif
