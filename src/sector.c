#ifndef SECTOR_C
#define SECTOR_C

#include "headers/utils.h"
#include "headers/sector.h"
#include "headers/statemap.h"
#include "headers/face.h"
#include "headers/shader.h"
#include "headers/constants.h"
#include "headers/list.h"

// Attributes here = height and texture id
static byte point_matches_sector_attributes(const Sector* const sector,
	const byte* const heightmap, const byte* const texture_id_map,
	const byte x, const byte y, const byte map_width) {

	return
		sample_map_point(heightmap, x, y, map_width) == sector -> visible_heights.max &&
		sample_map_point(texture_id_map, x, y, map_width) == sector -> texture_id;
}

// Gets length across, and then adds to area size y until out of map or length across not eq
static Sector form_sector_area(Sector sector, const StateMap traversed_points,
	const byte* const heightmap, const byte* const texture_id_map,
	const byte map_width, const byte map_height) {

	byte top_right_corner_x = sector.origin[0];
	const byte origin_y = sector.origin[1];

	while (top_right_corner_x < map_width &&
		!statemap_bit_is_set(traversed_points, top_right_corner_x, origin_y) &&
		point_matches_sector_attributes(&sector, heightmap, texture_id_map, top_right_corner_x, origin_y, map_width)) {

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
	const byte* const texture_id_map, const byte map_width, const byte map_height) {

	// `>> 3` = `/ 8`. Works pretty well for my maps.
	const buffer_size_t sector_amount_guess = (map_width * map_height) >> 3;
	List sectors = init_list(sector_amount_guess, Sector);

	/* StateMap used instead of a heightmap with null map points, because 1. less
	bytes used and 2. for forming faces, the original heightmap will need to be unmodified. */

	const StateMap traversed_points = init_statemap(map_width, map_height);

	for (byte y = 0; y < map_height; y++) {
		for (byte x = 0; x < map_width; x++) {
			if (statemap_bit_is_set(traversed_points, x, y)) continue;

			const byte texture_id = sample_map_point(texture_id_map, x, y, map_width);

			if (texture_id >= MAX_NUM_SECTOR_SUBTEXTURES)
				FAIL(TextureIDIsTooLarge, "Could not create a sector at map position {%u, %u} because the texture "
					"ID %u exceeds the maximum, which is %u", x, y, texture_id, MAX_NUM_SECTOR_SUBTEXTURES);

			const Sector seed_sector = {
				.texture_id = texture_id, .origin = {x, y}, .size = {0, 0},
				.visible_heights = {.min = 0, .max = sample_map_point(heightmap, x, y, map_width)},
				.face_range = {.start = 0, .length = 0}
			};

			const Sector sector = form_sector_area(seed_sector, traversed_points,
				heightmap, texture_id_map, map_width, map_height);

			push_ptr_to_list(&sectors, &sector);

			/* This is a simple optimization; the next `sector_width - 1`
			tiles will already be traversed, so this just skips those. */
			x += sector.size[0] - 1;
		}
	}

	deinit_statemap(traversed_points);
	return sectors;
}

////////// This next part concerns the drawing of sectors

void init_sector_draw_context(BatchDrawContext* const draw_context,
	List* const sectors_ref, const byte* const heightmap,
	const byte* const texture_id_map, const byte map_width, const byte map_height) {

	List sectors = generate_sectors_from_maps(heightmap, texture_id_map, map_width, map_height);

	/* This contains the actual vertex data for faces. `sectors.length * 3` gives a good guess for the
	face/sector ratio. Its ownership, after this function, goes to the draw context (which frees it). */
	List face_meshes = init_list(sectors.length * 3, face_mesh_component_t[components_per_face]);

	for (buffer_size_t i = 0; i < sectors.length; i++) {
		Sector* const sector_ref = ptr_to_list_index(&sectors, i);
		sector_ref -> face_range.start = face_meshes.length;

		const Sector sector = *sector_ref;
		const Face flat_face = {Flat, {sector.origin[0], sector.origin[1]}, {sector.size[0], sector.size[1]}};
		add_face_mesh_to_list(flat_face, sector.visible_heights.max, 0, sector.texture_id, &face_meshes);

		byte biggest_face_height = 0;
		init_vert_faces(sector, &face_meshes, heightmap, map_width, map_height, &biggest_face_height);

		sector_ref -> visible_heights.min = sector.visible_heights.max - biggest_face_height;
		sector_ref -> face_range.length = face_meshes.length - sector_ref -> face_range.start;
	}

	draw_context -> buffers.cpu = face_meshes;
	init_batch_draw_context_gpu_buffer(draw_context, face_meshes.length, bytes_per_face);
	draw_context -> shader = init_shader(ASSET_PATH("shaders/sector.vert"), NULL, ASSET_PATH("shaders/sector.frag"));

	draw_context -> vertex_spec = init_vertex_spec();
	use_vertex_spec(draw_context -> vertex_spec);
	define_vertex_spec_index(false, true, 0, 3, bytes_per_face_vertex, 0, FACE_MESH_COMPONENT_TYPENAME);
	define_vertex_spec_index(false, false, 1, 1, bytes_per_face_vertex, sizeof(face_mesh_component_t[3]), FACE_MESH_COMPONENT_TYPENAME);

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

// Used in main.c
void draw_all_sectors_for_shadow_map(const void* const param) {
	const BatchDrawContext* const sector_draw_context = (BatchDrawContext*) param;
	use_vertex_buffer(sector_draw_context -> buffers.gpu);
	use_vertex_spec(sector_draw_context -> vertex_spec);

	glDisableVertexAttribArray(1); // Not using the attribute at index 1

	GLint bytes_for_vertices;
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bytes_for_vertices);
	glBufferSubData(GL_ARRAY_BUFFER, 0, bytes_for_vertices, sector_draw_context -> buffers.cpu.data);

	const GLsizei num_vertices = bytes_for_vertices / bytes_per_face * vertices_per_face;
	glDrawArrays(GL_TRIANGLES, 0, num_vertices);

	glEnableVertexAttribArray(1);
}

static void draw_sectors(const BatchDrawContext* const draw_context,
	const CascadedShadowContext* const shadow_context, const Camera* const camera,
	const buffer_size_t num_visible_faces, const GLuint normal_map_set, const GLint screen_size[2]) {

	const GLuint shader = draw_context -> shader;

	static GLint
		camera_pos_world_space_id, light_dir_id, model_view_projection_id,
		one_over_screen_size_id, UV_translation_id, camera_view_id, light_view_projection_matrices_id;

	use_shader(shader);

	#define TONE_MAPPING_UNIFORM(param) INIT_UNIFORM_VALUE(tm_##param, shader, 1f, constants.lighting.tone_mapping.param)
	#define LIGHTING_UNIFORM(param, prefix) INIT_UNIFORM_VALUE(param, shader, prefix, constants.lighting.param)
	#define ARRAY_LIGHTING_UNIFORM(param, prefix) INIT_UNIFORM_VALUE(param, shader, prefix, 1, constants.lighting.param)

	ON_FIRST_CALL(
		INIT_UNIFORM(light_dir, shader);
		INIT_UNIFORM(camera_pos_world_space, shader);
		INIT_UNIFORM(model_view_projection, shader);
		INIT_UNIFORM(one_over_screen_size, shader);
		INIT_UNIFORM(UV_translation, shader);
		INIT_UNIFORM(camera_view, shader);
		INIT_UNIFORM(light_view_projection_matrices, shader);

		LIGHTING_UNIFORM(enable_tone_mapping, 1i);

		LIGHTING_UNIFORM(ambient, 1f);
		LIGHTING_UNIFORM(diffuse_strength, 1f);
		LIGHTING_UNIFORM(specular_strength, 1f);
		ARRAY_LIGHTING_UNIFORM(specular_exponent_domain, 2fv);

		TONE_MAPPING_UNIFORM(max_brightness);
		TONE_MAPPING_UNIFORM(linear_contrast);
		TONE_MAPPING_UNIFORM(linear_start);
		TONE_MAPPING_UNIFORM(linear_length);
		TONE_MAPPING_UNIFORM(black);
		TONE_MAPPING_UNIFORM(pedestal);

		LIGHTING_UNIFORM(noise_granularity, 1f);
		ARRAY_LIGHTING_UNIFORM(light_color, 3fv);

		const GLfloat epsilon = 0.005f;
		INIT_UNIFORM_VALUE(UV_translation_area, shader, 3fv, 2, (GLfloat*) (vec3[2]) {
			{4.0f + epsilon, 0.0f, 0.0f}, {6.0f - epsilon, 3.0f, 3.0f + epsilon}
		});

		const List* const split_dists = &shadow_context -> split_dists;
		INIT_UNIFORM_VALUE(cascade_split_distances, shader, 1fv, (GLsizei) split_dists -> length, split_dists -> data);

		use_texture(draw_context -> texture_set, shader, "texture_sampler", TexSet, SECTOR_FACE_TEXTURE_UNIT);
		use_texture(normal_map_set, shader, "normal_map_sampler", TexSet, SECTOR_NORMAL_MAP_TEXTURE_UNIT);
		use_texture(shadow_context -> depth_layers, shader, "shadow_cascade_sampler", TexSet, CASCADED_SHADOW_MAP_TEXTURE_UNIT);
	);

	const GLfloat t = SDL_GetTicks() / 3000.0f;
	UPDATE_UNIFORM(UV_translation, 2f, cosf(t), tanf(t));

	//////////

	#undef TONE_MAPPING_UNIFORM
	#undef LIGHTING_UNIFORM
	#undef ARRAY_LIGHTING_UNIFORM

	UPDATE_UNIFORM(camera_pos_world_space, 3fv, 1, camera -> pos);
	UPDATE_UNIFORM(light_dir, 3fv, 1, shadow_context -> light_dir);

	UPDATE_UNIFORM(model_view_projection, Matrix4fv, 1, GL_FALSE, &camera -> model_view_projection[0][0]);
	UPDATE_UNIFORM(one_over_screen_size, 2f, 1.0f / screen_size[0], 1.0f / screen_size[1]);

	////////// This little part concerns CSM

	UPDATE_UNIFORM(camera_view, Matrix4fv, 1, GL_FALSE, &camera -> view[0][0]);

	const List* const light_view_projection_matrices = &shadow_context -> light_view_projection_matrices;

	UPDATE_UNIFORM(light_view_projection_matrices, Matrix4fv,
		(GLsizei) light_view_projection_matrices -> length, GL_FALSE,
		light_view_projection_matrices -> data);

	//////////

	use_vertex_spec(draw_context -> vertex_spec);
	glDrawArrays(GL_TRIANGLES, 0, (GLsizei) (num_visible_faces * vertices_per_face));
}

// Returns the number of visible faces
static buffer_size_t fill_sector_vertex_buffer_with_visible_faces(
	const BatchDrawContext* const draw_context,
	const List* const sectors, const Camera* const camera) {

	use_vertex_buffer(draw_context -> buffers.gpu);

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
	const CascadedShadowContext* const shadow_context, const List* const sectors,
	const Camera* const camera, const GLuint normal_map_set, const GLint screen_size[2]) {

	const buffer_size_t num_visible_faces = fill_sector_vertex_buffer_with_visible_faces(draw_context, sectors, camera);

	if (num_visible_faces != 0) draw_sectors(draw_context, shadow_context,
		camera, num_visible_faces, normal_map_set, screen_size);
}

#endif
