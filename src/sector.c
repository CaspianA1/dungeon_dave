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
void draw_all_sectors_to_shadow_context(const SectorContext* const sector_context) {
	use_vertex_buffer(sector_context -> mesh_vertex_buffer);
	use_vertex_spec(sector_context -> mesh_vertex_spec);

	glDisableVertexAttribArray(1); // Not using the face info bit attribute at index 1

	//////////

	const List* const face_meshes_cpu = &sector_context -> mesh_cpu;
	const buffer_size_t num_face_meshes = face_meshes_cpu -> length;

	glBufferSubData(GL_ARRAY_BUFFER, 0, num_face_meshes * sizeof(face_mesh_t), face_meshes_cpu -> data);
	glDrawArrays(GL_TRIANGLES, 0, (GLsizei) (num_face_meshes * vertices_per_face));

	//////////

	glEnableVertexAttribArray(1);
}

static void internal_draw_sectors(
	const SectorContext* const sector_context,
	const CascadedShadowContext* const shadow_context,
	const Skybox* const skybox, const buffer_size_t num_visible_faces,
	const GLfloat curr_time_secs) {

	const GLuint shader = sector_context -> shader;
	static GLint UV_translation_id;

	use_shader(shader);

	#define LIGHTING_UNIFORM(param, prefix) INIT_UNIFORM_VALUE(param, shader, prefix, constants.lighting.param)
	#define ARRAY_LIGHTING_UNIFORM(param, prefix) INIT_UNIFORM_VALUE(param, shader, prefix, 1, constants.lighting.param)

	ON_FIRST_CALL( // TODO: remove this `ON_FIRST_CALL` block when possible
		INIT_UNIFORM(UV_translation, shader);

		const GLfloat epsilon = 0.005f;
		INIT_UNIFORM_VALUE(UV_translation_area, shader, 3fv, 2, (GLfloat*) (vec3[2]) {
			{4.0f + epsilon, 0.0f, 0.0f}, {6.0f - epsilon, 3.0f, 3.0f + epsilon}
		});

		use_texture(skybox -> diffuse_texture, shader, "environment_map_sampler", TexSkybox, TU_Skybox);
		use_texture(sector_context -> diffuse_texture_set, shader, "diffuse_sampler", TexSet, TU_SectorFaceDiffuse);
		use_texture(sector_context -> normal_map_set, shader, "normal_map_sampler", TexSet, TU_SectorFaceNormalMap);
		use_texture(shadow_context -> depth_layers, shader, "shadow_cascade_sampler", TexSet, TU_CascadedShadowMap);
	);

	const GLfloat t = curr_time_secs / 3.0f;
	UPDATE_UNIFORM(UV_translation, 2f, cosf(t), tanf(t));

	//////////

	#undef LIGHTING_UNIFORM
	#undef ARRAY_LIGHTING_UNIFORM

	use_vertex_spec(sector_context -> mesh_vertex_spec);
	glDrawArrays(GL_TRIANGLES, 0, (GLsizei) (num_visible_faces * vertices_per_face));
}

//////////

static buffer_size_t frustum_cull_sector_faces_into_gpu_buffer(
	const SectorContext* const sector_context, const vec4* const frustum_planes) {

	use_vertex_buffer(sector_context -> mesh_vertex_buffer);

	const List* const sectors = &sector_context -> sectors;
	const Sector* const sector_data = sectors -> data;
	const Sector* const out_of_bounds_sector = sector_data + sectors -> length;

	const List* const face_meshes_cpu = &sector_context -> mesh_cpu;
	const face_mesh_t* const face_meshes_cpu_data = face_meshes_cpu -> data;

	face_mesh_t* const face_meshes_gpu = glMapBufferRange(GL_ARRAY_BUFFER,
		0, face_meshes_cpu -> length * sizeof(face_mesh_t),
		GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT
		/* Only writing, so no `GL_MAP_READ_BIT`. The previous buffer contents
		don't matter, so I can invalidate the previous data in the array.
		TODO: possibly add `GL_MAP_UNSYNCHRONIZED_BIT` later? */
	);

	buffer_size_t num_visible_faces = 0;

	for (const Sector* sector = sector_data; sector < out_of_bounds_sector; sector++) {
		buffer_size_t num_visible_faces_in_group = 0;
		const buffer_size_t cpu_buffer_start_index = sector -> face_range.start;

		while (sector < out_of_bounds_sector) {
			////////// Checking to see if the sector is visible

			const byte *const origin = sector -> origin, *const size = sector -> size;

			const vec3 aabb[2] = {
				{origin[0], sector -> visible_heights.min, origin[1]},
				{origin[0] + size[0], sector -> visible_heights.max, origin[1] + size[1]}
			};

			if (!glm_aabb_frustum((vec3*) aabb, (vec4*) frustum_planes)) break;

			num_visible_faces_in_group += sector++ -> face_range.length;
		}

		if (num_visible_faces_in_group != 0) {
			memcpy(face_meshes_gpu + num_visible_faces,
				face_meshes_cpu_data + cpu_buffer_start_index,
				num_visible_faces_in_group * sizeof(face_mesh_t));

			num_visible_faces += num_visible_faces_in_group;
		}
	}

	glUnmapBuffer(GL_ARRAY_BUFFER); // If looking out at the distance with no sectors, why do any state switching at all?
	return num_visible_faces;
}

//////////

// This is just a utility function
void draw_sectors(const SectorContext* const sector_context,
	const CascadedShadowContext* const shadow_context,
	const Skybox* const skybox, const Camera* const camera,
	const GLfloat curr_time_secs) {

	const buffer_size_t num_visible_faces = frustum_cull_sector_faces_into_gpu_buffer(sector_context, camera -> frustum_planes);

	// If looking out at the distance with no sectors, why do any state switching at all?
	if (num_visible_faces != 0) internal_draw_sectors(sector_context, shadow_context, skybox, num_visible_faces, curr_time_secs);
}

////////// Initialization and deinitialization

// TODO: pass in only texture paths to this
SectorContext init_sector_context(const byte* const heightmap,
	const byte* const texture_id_map, const byte map_width, const byte map_height,
	const GLuint diffuse_texture_set, const NormalMapConfig* const normal_map_config) {

	const List sectors = generate_sectors_from_maps(heightmap, texture_id_map, map_width, map_height);

	SectorContext sector_context = {
		.mesh_vertex_buffer = init_gpu_buffer(),
		.mesh_vertex_spec = init_vertex_spec(),
		.diffuse_texture_set = diffuse_texture_set,
		.normal_map_set = init_normal_map_from_diffuse_texture_set(diffuse_texture_set, normal_map_config),
		.shader = init_shader(ASSET_PATH("shaders/sector.vert"), NULL, ASSET_PATH("shaders/sector.frag")),

		.mesh_cpu = init_face_meshes_from_sectors(&sectors, heightmap, map_width, map_height),
		.sectors = sectors
	};

	use_vertex_buffer(sector_context.mesh_vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sector_context.mesh_cpu.length * sizeof(face_mesh_t), NULL, GL_DYNAMIC_DRAW);

	enum {vpt = vertices_per_triangle};
	const GLenum typename = FACE_MESH_COMPONENT_TYPENAME;

	use_vertex_spec(sector_context.mesh_vertex_spec);
	define_vertex_spec_index(false, true, 0, vpt, sizeof(face_vertex_t), 0, typename); // Position
	define_vertex_spec_index(false, false, 1, 1, sizeof(face_vertex_t), sizeof(face_mesh_component_t[vpt]), typename); // Face info

	return sector_context;
}

void deinit_sector_context(const SectorContext* const sector_context) {
	deinit_gpu_buffer(sector_context -> mesh_vertex_buffer);
	deinit_vertex_spec(sector_context -> mesh_vertex_spec);

	deinit_texture(sector_context -> diffuse_texture_set);
	deinit_texture(sector_context -> normal_map_set);

	deinit_shader(sector_context -> shader);

	deinit_list(sector_context -> mesh_cpu);
	deinit_list(sector_context -> sectors);
}

#endif
