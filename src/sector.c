#ifndef SECTOR_C
#define SECTOR_C

#include "headers/sector.h"
#include "headers/statemap.h"
#include "headers/buffer_defs.h"
#include "headers/list.h"
#include "headers/texture.h"
#include "headers/face.h"
#include "headers/shader.h"
#include "headers/constants.h"

// Attributes here are height and texture id
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

	// Now, `sector.size[0]` equals the first horizontal span length where equal attributes were found
	for (byte y = origin_y; y < map_height; y++, sector.size[1]++) {
		for (byte x = sector.origin[0]; x < top_right_corner_x; x++) {
			if (!point_matches_sector_attributes(&sector, heightmap, texture_id_map, x, y, map_width)) {

				set_statemap_area(traversed_points, (buffer_size_t[4]) {
					sector.origin[0], sector.origin[1], sector.size[0], sector.size[1]
				});

				goto end;
			}
		}
	}

	end: return sector;
}

List generate_sectors_from_maps(const byte* const heightmap,
	const byte* const texture_id_map, const byte map_width, const byte map_height) {

	// `>> 3` = `/ 8`. Works pretty well for my maps. TODO: make this a constant somewhere.
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
			tiles will already be marked traversed, so this just skips those. */
			x += sector.size[0] - 1;
		}
	}

	deinit_statemap(traversed_points);
	return sectors;
}

////////// This next part concerns the creation of sa sector context + the drawing of sectors

void init_sector_draw_context(BatchDrawContext* const draw_context, List* const sectors,
	const byte* const heightmap, const byte* const texture_id_map, const byte map_width, const byte map_height) {

	*sectors = generate_sectors_from_maps(heightmap, texture_id_map, map_width, map_height);

	List* const face_meshes_ref = &draw_context -> buffers.cpu;
	*face_meshes_ref = init_face_meshes_from_sectors(sectors, heightmap, map_width, map_height);

	init_batch_draw_context_gpu_buffer(draw_context, face_meshes_ref -> length, sizeof(face_mesh_t));
	draw_context -> shader = init_shader(ASSET_PATH("shaders/sector.vert"), NULL, ASSET_PATH("shaders/sector.frag"));

	draw_context -> vertex_spec = init_vertex_spec();
	use_vertex_spec(draw_context -> vertex_spec);

	enum {vpt = vertices_per_triangle};
	define_vertex_spec_index(false, true, 0, vpt, sizeof(face_vertex_t), 0, FACE_MESH_COMPONENT_TYPENAME); // Position
	define_vertex_spec_index(false, false, 1, 1, sizeof(face_vertex_t), sizeof(face_mesh_component_t[vpt]), FACE_MESH_COMPONENT_TYPENAME); // Face info
}

static bool sector_in_view_frustum(const Sector sector, const vec4 frustum_planes[planes_per_frustum]) {
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

	const GLsizei num_vertices = bytes_for_vertices / (GLint) sizeof(face_mesh_t) * vertices_per_face;
	glDrawArrays(GL_TRIANGLES, 0, num_vertices);

	glEnableVertexAttribArray(1);
}

static void draw_sectors(const BatchDrawContext* const draw_context,
	const CascadedShadowContext* const shadow_context, const Camera* const camera,
	const buffer_size_t num_visible_faces, const GLuint normal_map_set, const GLint screen_size[2]) {

	const GLuint shader = draw_context -> shader;

	static GLint
		camera_pos_world_space_id, light_dir_id, view_projection_id,
		one_over_screen_size_id, UV_translation_id, camera_view_id, light_view_projection_matrices_id;

	use_shader(shader);

	#define TONE_MAPPING_UNIFORM(param) INIT_UNIFORM_VALUE(tm_##param, shader, 1f, constants.lighting.tone_mapping.param)
	#define LIGHTING_UNIFORM(param, prefix) INIT_UNIFORM_VALUE(param, shader, prefix, constants.lighting.param)
	#define ARRAY_LIGHTING_UNIFORM(param, prefix) INIT_UNIFORM_VALUE(param, shader, prefix, 1, constants.lighting.param)

	ON_FIRST_CALL(
		INIT_UNIFORM(light_dir, shader);
		INIT_UNIFORM(camera_pos_world_space, shader);
		INIT_UNIFORM(view_projection, shader);
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

	UPDATE_UNIFORM(view_projection, Matrix4fv, 1, GL_FALSE, &camera -> view_projection[0][0]);
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

	const face_mesh_t* const face_meshes_cpu = draw_context -> buffers.cpu.data;
	face_mesh_t* const face_meshes_gpu = init_mapping_for_culled_batching(draw_context);

	buffer_size_t num_visible_faces = 0;

	for (const Sector* sector = sector_data; sector < out_of_bounds_sector; sector++) {
		buffer_size_t num_visible_faces_in_group = 0;
		const buffer_size_t initial_face_index = sector -> face_range.start;

		while (sector < out_of_bounds_sector && sector_in_view_frustum(*sector, frustum_planes))
			num_visible_faces_in_group += sector++ -> face_range.length;

		if (num_visible_faces_in_group != 0) {
			memcpy(face_meshes_gpu + num_visible_faces,
				face_meshes_cpu + initial_face_index,
				num_visible_faces_in_group * sizeof(face_mesh_t));

			num_visible_faces += num_visible_faces_in_group;
		}
	}

	deinit_current_mapping_for_culled_batching();
	return num_visible_faces;
}

// This is just a utility function
void draw_visible_sectors(const BatchDrawContext* const draw_context,
	const CascadedShadowContext* const shadow_context, const List* const sectors,
	const Camera* const camera, const GLuint normal_map_set, const GLint screen_size[2]) {

	const buffer_size_t num_visible_faces = fill_sector_vertex_buffer_with_visible_faces(draw_context, sectors, camera);

	// If looking out at the distance with no sectors, why do any state switching at all?
	if (num_visible_faces != 0) draw_sectors(draw_context, shadow_context,
		camera, num_visible_faces, normal_map_set, screen_size);
}

#endif
