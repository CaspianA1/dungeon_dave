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
#include "headers/normal_map_generation.h"

/* Drawing sectors to the shadow map:

During initialization:
	- Generate another sector buffer that contains all sectors, but without their face ids
	- Additional faces for map edges are generated too
	- Face pre-culling based on the face type could be done too, but that could be considered extra credit
	- Keep that alternate mesh in a GL_STATIC_DRAW buffer
	- Another vertex spec for that is then kept, since no skipping of positions to get the face id is needed

	Advantages:
		- No resubmitting of data to the plain sector buffer
		- Map edges not drawn by the plain sector shader
		- Rendering the shadow buffer should be faster with a GL_STATIC_DRAW buffer, and no face ids used
	Disadvantages:
		- One more vertex buffer and spec used

Alternate method:
	- Keep sector edge data in the same buffer
	- Resubmit sector data each time
	- Same vertex spec used

	Advantages:
		- Only one vertex buffer and spec for sectors
	Disasvantages:
		- GL_DYNAMIC_DRAW buffer may be slower
		- Map edge faces will never be seen because they'll be backfacing

Compromise:
	- Keep the sector edge faces in the end of the sector GPU buffer
	- Draw them when rendering the shadow map, and not when rendering culled faces otherwise
	- For mapping the buffer for culling, make sure that the contents outside the range are preserved
	- Generate null face ids for those edge faces, since they aren't used
*/

/*
- TODO: fix the bug where pushing oneself in a corner, and looking up and down with the right yaw clips a whole sector face
- To fix this whole depth clamping mess,
	1. Use a different projection matrix for the weapon with a much nearer clip dist
	2. Choose a reasonably near clip dist for the scene
	3. Disable depth clamping

Also, figure out why making the near clip dist smaller warps the weapon much more for rotations.
*/

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
			if (!point_matches_sector_attributes(&sector, heightmap, texture_id_map, x, y, map_width))
				goto done;
		}
	}

	done:

	set_statemap_area(traversed_points, (buffer_size_t[4]) {
		sector.origin[0], sector.origin[1], sector.size[0], sector.size[1]
	});

	return sector;
}

static List generate_sectors_from_maps(const byte* const heightmap,
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

// Used in main.c
void draw_all_sectors_to_shadow_context(const BatchDrawContext* const sector_draw_context) {
	use_vertex_buffer(sector_draw_context -> buffers.gpu);
	use_vertex_spec(sector_draw_context -> vertex_spec);

	glDisableVertexAttribArray(1); // Not using the face info bit attribute at index 1

	//////////

	const List* const face_meshes = &sector_draw_context -> buffers.cpu;
	const buffer_size_t num_face_meshes = face_meshes -> length;

	glBufferSubData(GL_ARRAY_BUFFER, 0, num_face_meshes * sizeof(face_mesh_t), face_meshes -> data);
	glDrawArrays(GL_TRIANGLES, 0, (GLsizei) (num_face_meshes * vertices_per_face));

	//////////

	glEnableVertexAttribArray(1);
}

static void draw_sectors(
	const SectorContext* const sector_context,
	const CascadedShadowContext* const shadow_context,
	const Skybox* const skybox, const Camera* const camera,
	const buffer_size_t num_visible_faces, const GLfloat curr_time_secs) {

	const BatchDrawContext* const draw_context = &sector_context -> draw_context;

	const GLuint shader = draw_context -> shader;

	static GLint UV_translation_id, camera_view_id, light_view_projection_matrices_id;

	use_shader(shader);

	#define LIGHTING_UNIFORM(param, prefix) INIT_UNIFORM_VALUE(param, shader, prefix, constants.lighting.param)
	#define ARRAY_LIGHTING_UNIFORM(param, prefix) INIT_UNIFORM_VALUE(param, shader, prefix, 1, constants.lighting.param)

	const GLsizei num_cascades = shadow_context -> num_cascades;

	ON_FIRST_CALL( // TODO: remove this `ON_FIRST_CALL` block when possible
		INIT_UNIFORM(UV_translation, shader);
		INIT_UNIFORM(camera_view, shader);
		INIT_UNIFORM(light_view_projection_matrices, shader);

		const GLfloat epsilon = 0.005f;
		INIT_UNIFORM_VALUE(UV_translation_area, shader, 3fv, 2, (GLfloat*) (vec3[2]) {
			{4.0f + epsilon, 0.0f, 0.0f}, {6.0f - epsilon, 3.0f, 3.0f + epsilon}
		});

		INIT_UNIFORM_VALUE(cascade_split_distances, shader, 1fv, num_cascades - 1, shadow_context -> split_dists);

		use_texture(skybox -> diffuse_texture, shader, "environment_map_sampler", TexSkybox, TU_Skybox);
		use_texture(draw_context -> texture_set, shader, "diffuse_sampler", TexSet, TU_SectorFaceDiffuse);
		use_texture(sector_context -> face_normal_map_set, shader, "normal_map_sampler", TexSet, TU_SectorFaceNormalMap);
		use_texture(shadow_context -> depth_layers, shader, "shadow_cascade_sampler", TexSet, TU_CascadedShadowMap);
	);

	const GLfloat t = curr_time_secs / 3.0f;
	UPDATE_UNIFORM(UV_translation, 2f, cosf(t), tanf(t));

	//////////

	#undef LIGHTING_UNIFORM
	#undef ARRAY_LIGHTING_UNIFORM

	////////// This little part concerns CSM

	UPDATE_UNIFORM(camera_view, Matrix4fv, 1, GL_FALSE, &camera -> view[0][0]);
	UPDATE_UNIFORM(light_view_projection_matrices, Matrix4fv, num_cascades, GL_FALSE, (GLfloat*) shadow_context -> light_view_projection_matrices);

	//////////

	use_vertex_spec(draw_context -> vertex_spec);
	glDrawArrays(GL_TRIANGLES, 0, (GLsizei) (num_visible_faces * vertices_per_face));
}

////////// These functions are for frustum culling

static void make_aabb(const byte* const typeless_sector, vec3 aabb[2]) {
	const Sector sector = *(Sector*) typeless_sector;

	GLfloat *const min = aabb[0], *const max = aabb[1];

	min[0] = sector.origin[0];
	min[1] = sector.visible_heights.min;
	min[2] = sector.origin[1];

	max[0] = min[0] + sector.size[0];
	max[1] = min[1] + sector.visible_heights.max - sector.visible_heights.min;
	max[2] = min[2] + sector.size[1];
}

static buffer_size_t get_renderable_index_from_cullable(const byte* const typeless_sector, const byte* const typeless_first_sector) {
	(void) typeless_first_sector;
	return ((Sector*) typeless_sector) -> face_range.start;
}

static buffer_size_t get_num_renderable_from_cullable(const byte* const typeless_sector) {
	return ((Sector*) typeless_sector) -> face_range.length;
}

//////////

// This is just a utility function
void draw_visible_sectors(const SectorContext* const sector_context,
	const CascadedShadowContext* const shadow_context,
	const Skybox* const skybox, const Camera* const camera,
	const GLfloat curr_time_secs) {

	const buffer_size_t num_visible_faces = cull_from_frustum_into_gpu_buffer(
		&sector_context -> draw_context, sector_context -> sectors, camera -> frustum_planes,
		make_aabb, get_renderable_index_from_cullable, get_num_renderable_from_cullable
	);

	// If looking out at the distance with no sectors, why do any state switching at all?
	if (num_visible_faces != 0) draw_sectors(sector_context,
		shadow_context, skybox, camera, num_visible_faces, curr_time_secs);
}

////////// Initialization and deinitialization

SectorContext init_sector_context(const byte* const heightmap, const byte* const texture_id_map,
	const byte map_width, const byte map_height, const bool apply_normal_map_blur, const GLuint texture_set) {

	SectorContext sector_context = {
		.face_normal_map_set = init_normal_map_set_from_texture_set(texture_set, apply_normal_map_blur),

		.draw_context = {
			.vertex_spec = init_vertex_spec(), .texture_set = texture_set,
			.shader = init_shader(ASSET_PATH("shaders/sector.vert"), NULL, ASSET_PATH("shaders/sector.frag"))
		},

		.sectors = generate_sectors_from_maps(heightmap, texture_id_map, map_width, map_height)
	};

	List* const face_meshes_cpu = &sector_context.draw_context.buffers.cpu;
	*face_meshes_cpu = init_face_meshes_from_sectors(&sector_context.sectors, heightmap, map_width, map_height);
	init_batch_draw_context_gpu_buffer(&sector_context.draw_context, face_meshes_cpu -> length, sizeof(face_mesh_t));

	enum {vpt = vertices_per_triangle};
	const GLenum typename = FACE_MESH_COMPONENT_TYPENAME;

	use_vertex_spec(sector_context.draw_context.vertex_spec);
	define_vertex_spec_index(false, true, 0, vpt, sizeof(face_vertex_t), 0, typename); // Position
	define_vertex_spec_index(false, false, 1, 1, sizeof(face_vertex_t), sizeof(face_mesh_component_t[vpt]), typename); // Face info

	return sector_context;
}

void deinit_sector_context(const SectorContext* const sector_context) {
	deinit_batch_draw_context(&sector_context -> draw_context);
	deinit_list(sector_context -> sectors);
	deinit_texture(sector_context -> face_normal_map_set);
}

#endif
