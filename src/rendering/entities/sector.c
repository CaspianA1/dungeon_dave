#include "rendering/entities/sector.h"
#include "rendering/entities/face.h"
#include "utils/statemap.h"
#include "utils/texture.h"
#include "utils/shader.h"
#include "utils/opengl_wrappers.h"
#include "data/constants.h"

static const TextureType displacement_map_texture_type = TexPlainRect;

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

- For dynamic sectors, perhaps have a 2D floating-point map that represents the displacement height of vertices,
	so that sectors can be pulled up or down from the ground

- Steep parallax mapping, or another technique under that umbrella
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
	buffer_size_t sector_amount_guess = (map_width * map_height) >> 3;
	// If the map size is small enough, an incorrect guess of zero can happen
	if (sector_amount_guess == 0) sector_amount_guess = 1;

	List sectors = init_list(sector_amount_guess, Sector);

	/* StateMap used instead of a heightmap with null map points, because 1. less
	bytes used and 2. for forming faces, the original heightmap will need to be unmodified. */

	const StateMap traversed_points = init_statemap(map_width, map_height);

	for (byte y = 0; y < map_height; y++) {
		for (byte x = 0; x < map_width; x++) {
			if (statemap_bit_is_set(traversed_points, x, y)) continue;

			const byte texture_id = sample_map_point(texture_id_map, x, y, map_width);

			if (texture_id >= MAX_NUM_SECTOR_SUBTEXTURES)
				FAIL(TextureIDIsTooLarge, "Could not create a sector at map position {%hhu, %hhu} because the texture "
					"ID %hhu exceeds the maximum, which is %hhu", x, y, texture_id, MAX_NUM_SECTOR_SUBTEXTURES);

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

static buffer_size_t frustum_cull_sector_faces_into_gpu_buffer(
	const SectorContext* const sector_context, const vec4* const frustum_planes) {

	const List* const sectors = &sector_context -> sectors;
	const Sector* const sector_data = sectors -> data;
	const Sector* const out_of_bounds_sector = sector_data + sectors -> length;

	const List* const face_meshes_cpu = &sector_context -> mesh_cpu;
	const face_mesh_t* const face_meshes_cpu_data = face_meshes_cpu -> data;

	face_mesh_t* const face_meshes_gpu = init_vertex_buffer_memory_mapping(
		sector_context -> drawable.vertex_buffer,
		face_meshes_cpu -> length * sizeof(face_mesh_t), true
	);

	buffer_size_t num_visible_faces = 0;

	for (const Sector* sector = sector_data; sector < out_of_bounds_sector; sector++) {
		buffer_size_t num_visible_faces_in_group = 0;
		const buffer_size_t cpu_buffer_start_index = sector -> face_range.start;

		while (sector < out_of_bounds_sector) {
			////////// Checking to see if the sector is visible

			(void) frustum_planes;
			// TODO: add this back, but accounting for displaced sector heights
			/*
			const byte *const origin = sector -> origin, *const size = sector -> size;

			const vec3 aabb[2] = {
				{origin[0], sector -> visible_heights.min, origin[1]},
				{origin[0] + size[0], sector -> visible_heights.max, origin[1] + size[1]}
			};

			if (!glm_aabb_frustum((vec3*) aabb, (vec4*) frustum_planes)) break;
			*/

			num_visible_faces_in_group += sector++ -> face_range.length;
		}

		if (num_visible_faces_in_group != 0) {
			memcpy(face_meshes_gpu + num_visible_faces,
				face_meshes_cpu_data + cpu_buffer_start_index,
				num_visible_faces_in_group * sizeof(face_mesh_t));

			num_visible_faces += num_visible_faces_in_group;
		}
	}

	deinit_vertex_buffer_memory_mapping();
	return num_visible_faces;
}

////////// Uniform updating

typedef struct {
	const SectorContext* const sector_context;
	const Skybox* const skybox;
	const CascadedShadowContext* const shadow_context;
	const AmbientOcclusionMap ao_map;
} UniformUpdaterParams;

static void update_uniforms(const Drawable* const drawable, const void* const param) {
	const UniformUpdaterParams typed_params = *(UniformUpdaterParams*) param;
	const GLuint shader = drawable -> shader;

	ON_FIRST_CALL( // TODO: remove this `ON_FIRST_CALL` block when possible
		use_texture_in_shader(typed_params.skybox -> diffuse_texture, shader, "environment_map_sampler", TexSkybox, TU_Skybox);
		use_texture_in_shader(typed_params.sector_context -> drawable.diffuse_texture, shader, "diffuse_sampler", TexSet, TU_SectorFaceDiffuse);
		use_texture_in_shader(typed_params.sector_context -> normal_map_set, shader, "normal_map_sampler", TexSet, TU_SectorFaceNormalMap);
		use_texture_in_shader(typed_params.shadow_context -> depth_layers, shader, "shadow_cascade_sampler", shadow_map_texture_type, TU_CascadedShadowMap);
		use_texture_in_shader(typed_params.ao_map, shader, "ambient_occlusion_sampler", TexVolumetric, TU_AmbientOcclusionMap);
		use_texture_in_shader(typed_params.sector_context -> y_displacement_map.gpu, shader, "y_displacement_sampler", displacement_map_texture_type, TU_Y_DisplacementMap);
	);
}

static void define_vertex_spec(void) {
	enum {vpt = vertices_per_triangle};
	const GLenum typename = FACE_COMPONENT_TYPENAME;

	define_vertex_spec_index(false, true, 0, vpt, sizeof(face_vertex_t), 0, typename); // Position
	define_vertex_spec_index(false, false, 1, 1, sizeof(face_vertex_t), sizeof(face_component_t[vpt]), typename); // Face info
}

//////////

static void validate_sector_displacement_area(const List* const sectors,
	List* const mesh_cpu, const SectorDisplacementArea* const area) {

	////////// Checking that the area covers a valid sector

	const byte *const origin = area -> origin, *const size = area -> size;

	const byte
		origin_x = origin[0], origin_z = origin[1],
		size_x = size[0], size_z = size[1];

	const Sector* matching_sector = NULL;

	#define BASE_FAILURE_STRING "Could not find a sector that matches "\
			"the area of origin {%hhu, %hhu}, and size {%hhu, %hhu}"

	// TODO: allow multi-sector coverage
	LIST_FOR_EACH(0, sectors, untyped_sector, _,
		const Sector* const sector = (Sector*) untyped_sector;
		const byte *const sector_origin = sector -> origin, *const sector_size = sector -> size;

		const bool origin_matches = sector_origin[0] == origin_x && sector_origin[1] == origin_z;

		if (origin_matches) {
			const byte sector_width = sector_size[0], sector_height = sector_size[1];

			if (sector_width == size_x && sector_height == size_z) { // Each sector has its own origin
				matching_sector = sector;
				break;
			}
			else
				/* Failure is appropriate here since no other sector will have the
				requested origin, so this is as close as you can get to the requested area */
				FAIL(DisplaceHeightmapPortion, BASE_FAILURE_STRING
					" The origin did match, though; and the sector size was {%hhu, %hhu}",
					origin_x, origin_z, size_x, size_z, sector_width, sector_height);
		}
	);

	if (matching_sector == NULL) FAIL(DisplaceHeightmapPortion, BASE_FAILURE_STRING, origin_x, origin_z, size_x, size_z);

	#undef BASE_FAILURE_STRING

	//////////

	const byte mask_for_dynamic_sector = 1u << 7u;

	const buffer_size_t mesh_cpu_start = matching_sector -> face_range.start;
	const buffer_size_t mesh_cpu_end = mesh_cpu_start + matching_sector -> face_range.length;

	face_mesh_t* const face_meshes = mesh_cpu -> data;

	for (buffer_size_t mesh_index = mesh_cpu_start; mesh_index < mesh_cpu_end; mesh_index++) {
		face_vertex_t* const face_mesh = face_meshes[mesh_index];

		for (byte rel_vertex_index = 0; rel_vertex_index < vertices_per_face; rel_vertex_index++)
			face_mesh[rel_vertex_index][3] |= mask_for_dynamic_sector;
	}
}

static void write_area_to_displacement_map(const SectorDisplacementArea area,
	GLfloat* const displacement_map, const byte map_width) {

	const byte
		origin_x = area.origin[0], origin_z = area.origin[1],
		size_x = area.size[0], size_z = area.size[1];

	for (byte z = origin_z; z < origin_z + size_z; z++) {
		GLfloat* const row = displacement_map + z * map_width;
		for (byte x = origin_x; x < origin_x + size_x; x++)
			row[x] = area.amount;
	}
}

////////// Initialization, deinitialization, and rendering

SectorContext init_sector_context(const byte* const heightmap,
	const byte* const texture_id_map, const byte map_width, const byte map_height,
	const GLchar* const* const texture_paths, const GLsizei num_textures,
	const GLsizei texture_size, const NormalMapConfig* const normal_map_config) {

	const List sectors = generate_sectors_from_maps(heightmap, texture_id_map, map_width, map_height);
	List mesh_cpu = init_face_meshes_from_sectors(&sectors, heightmap, map_width, map_height);

	//////////

	GLfloat* const displacement_map = clearing_alloc(map_width * map_height, sizeof(GLfloat));

	const SectorDisplacementArea areas[] = {
		{{4, 1}, {2, 2}, 0.5f},
		{{21, 30}, {8, 2}, 1.0f},
		{{15, 30}, {1, 1}, 0.5f}
	};

	for (buffer_size_t i = 0; i < ARRAY_LENGTH(areas); i++) {
		const SectorDisplacementArea* const area = areas + i;

		validate_sector_displacement_area(&sectors, &mesh_cpu, area);
		write_area_to_displacement_map(*area, displacement_map, map_width);
	}

	const GLuint displacement_texture = preinit_texture(displacement_map_texture_type, TexNonRepeating, TexNearest, TexNearest, true);
	init_texture_data(displacement_map_texture_type, (GLsizei[]) {map_width, map_height}, GL_RED, GL_R32F, GL_FLOAT, displacement_map); // TODO: lower this bit depth, if needed

	//////////

	const GLuint diffuse_texture_set = init_texture_set(
		false, TexRepeating, OPENGL_SCENE_MAG_FILTER, OPENGL_SCENE_MIN_FILTER,
		num_textures, 0, texture_size, texture_size, texture_paths, NULL
	);

	return (SectorContext) {
		.drawable = init_drawable_with_vertices(
			define_vertex_spec, (uniform_updater_t) update_uniforms, GL_DYNAMIC_DRAW, GL_TRIANGLES,
			(List) {.data = NULL, .item_size = mesh_cpu.item_size, .length = mesh_cpu.length},
			init_shader(ASSET_PATH("shaders/sector.vert"), NULL, ASSET_PATH("shaders/sector.frag"), NULL),
			diffuse_texture_set
		),

		.normal_map_set = init_normal_map_from_diffuse_texture(diffuse_texture_set, TexSet, normal_map_config),
		.mesh_cpu = mesh_cpu, .sectors = sectors,

		.y_displacement_map = {.cpu = displacement_map, .gpu = displacement_texture}
	};
}

void deinit_sector_context(const SectorContext* const sector_context) {
	dealloc(sector_context -> y_displacement_map.cpu);
	deinit_texture(sector_context -> y_displacement_map.gpu);

	deinit_list(sector_context -> mesh_cpu);
	deinit_list(sector_context -> sectors);
	deinit_drawable(sector_context -> drawable);
	deinit_texture(sector_context -> normal_map_set);
}

// Used in main.c
void draw_all_sectors_to_shadow_context(const SectorContext* const sector_context) {
	const Drawable* const drawable = &sector_context -> drawable;
	const List* const face_meshes_cpu = &sector_context -> mesh_cpu;
	const buffer_size_t num_face_meshes = face_meshes_cpu -> length;

	use_vertex_buffer(drawable -> vertex_buffer); // TODO: don't resubmit this data every frame
	reinit_vertex_buffer_data(num_face_meshes, sizeof(face_mesh_t), face_meshes_cpu -> data);
	use_vertex_spec(drawable -> vertex_spec);

	WITHOUT_VERTEX_SPEC_INDEX(1, // Not using the face info bit attribute
		draw_drawable(*drawable, num_face_meshes * vertices_per_face, 0, NULL, OnlyDraw);
	);
}

void draw_sectors(const SectorContext* const sector_context,
	const CascadedShadowContext* const shadow_context, const Skybox* const skybox,
	const vec4 frustum_planes[planes_per_frustum], const AmbientOcclusionMap ao_map) {

	const buffer_size_t num_visible_faces = frustum_cull_sector_faces_into_gpu_buffer(sector_context, frustum_planes);

	// If looking out at the distance with no sectors, why do any state switching at all?
	if (num_visible_faces != 0)
		draw_drawable(sector_context -> drawable, num_visible_faces * vertices_per_face, 0,
			&(UniformUpdaterParams) {sector_context, skybox, shadow_context, ao_map},
			UseShaderPipeline | BindVertexSpec);
}
