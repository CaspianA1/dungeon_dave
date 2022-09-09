#include "rendering/entities/sector.h"
#include "rendering/entities/face.h"
#include "utils/statemap.h"
#include "utils/texture.h"
#include "utils/shader.h"
#include "utils/opengl_wrappers.h"
#include "data/constants.h"

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

- Steep parallax mapping, parallax occlusion mapping, or another technique under that umbrella
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
	const byte x, const byte y, const byte map_width, const byte texture_id) {

	return
		sample_map_point(heightmap, x, y, map_width) == sector -> visible_heights.max &&
		sample_map_point(texture_id_map, x, y, map_width) == texture_id;
}

// Gets length across, and then adds to area size y until out of map or length across not eq
static void form_sector_area(Sector* const sector, const StateMap traversed_points,
	const byte* const heightmap, const byte* const texture_id_map,
	const byte map_width, const byte map_height, const byte texture_id) {

	const byte* const origin = sector -> origin;
	byte* const size = sector -> size;

	byte top_x = origin[0];
	const byte origin_y = origin[1];

	while (top_x < map_width && !statemap_bit_is_set(traversed_points, top_x, origin_y) &&
		point_matches_sector_attributes(sector, heightmap, texture_id_map, top_x, origin_y, map_width, texture_id)) {

		size[0]++;
		top_x++;
	}

	// Now, `sector.size[0]` equals the first horizontal span length where equal attributes were found
	for (byte y = origin_y; y < map_height; y++, size[1]++) {
		for (byte x = origin[0]; x < top_x; x++) { // Here, `top_x` equals the top right corner
			if (!point_matches_sector_attributes(sector, heightmap, texture_id_map, x, y, map_width, texture_id))
				goto done;
		}
	}

	done:

	set_statemap_area(traversed_points, (buffer_size_t[4]) {origin[0], origin[1], size[0], size[1]});
}

// static List generate_sectors_from_maps(const byte* const heightmap,
static void generate_sectors_and_face_meshes_from_maps(
	List* const sectors, List* const face_meshes,
	const byte* const heightmap, const byte* const texture_id_map, const byte map_width, const byte map_height) {

	//////////

	/* `>> 3` = `/ 8`. Works pretty well for my maps. TODO: make to a constant.
	If the map size is small enough, an incorrect guess of zero can happen. */
	buffer_size_t sector_amount_guess = (map_width * map_height) >> 3;
	if (sector_amount_guess == 0) sector_amount_guess = 1;
	*sectors = init_list(sector_amount_guess, Sector);

	//////////

	/* This contains the actual data for faces. `num_sectors * 3` gives a good
	guess for the face/sector ratio. TODO: make this a constant somewhere. */
	*face_meshes = init_list(sector_amount_guess * 3, face_mesh_t);

	//////////

	/* A StateMap is used instead of a heightmap with null map points, because 1. less
	bytes used and 2. for forming faces, the original heightmap will need to be unmodified. */
	const StateMap traversed_points = init_statemap(map_width, map_height);

	for (byte y = 0; y < map_height; y++) {
		for (byte x = 0; x < map_width; x++) {
			if (statemap_bit_is_set(traversed_points, x, y)) continue;

			const byte texture_id = sample_map_point(texture_id_map, x, y, map_width);

			if (texture_id >= MAX_NUM_SECTOR_SUBTEXTURES)
				FAIL(TextureIDIsTooLarge, "Could not create a sector at map position {%hhu, %hhu} because the texture "
					"ID %hhu exceeds the maximum, which is %hhu", x, y, texture_id, (byte) (MAX_NUM_SECTOR_SUBTEXTURES - 1u));

			Sector sector = {
				.origin = {x, y}, .size = {0, 0},
				.visible_heights = {.min = 0, .max = sample_map_point(heightmap, x, y, map_width)},
				.face_range = {.start = 0, .length = 0}
			};

			form_sector_area(&sector, traversed_points, heightmap, texture_id_map, map_width, map_height, texture_id);

			////////// Setting face mesh metadata + initing sector faces

			sector.face_range.start = face_meshes -> length;

			byte biggest_face_height;
			init_mesh_for_sector(&sector, face_meshes, &biggest_face_height, heightmap, map_width, map_height, texture_id);

			sector.visible_heights.min = sector.visible_heights.max - biggest_face_height;
			sector.face_range.length = face_meshes -> length - sector.face_range.start;

			//////////

			push_ptr_to_list(sectors, &sector);

			/* This is a simple optimization; the next `sector_width - 1`
			tiles will already be marked traversed, so this just skips those. */
			x += sector.size[0] - 1;
		}
	}

	deinit_statemap(traversed_points);
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

	deinit_vertex_buffer_memory_mapping();
	return num_visible_faces;
}

////////// Uniform updating

typedef struct {
	const Skybox* const skybox;
	const SectorContext* const sector_context;
	const CascadedShadowContext* const shadow_context;
	const AmbientOcclusionMap ao_map;
} UniformUpdaterParams;

static void update_uniforms(const Drawable* const drawable, const void* const param) {
	ON_FIRST_CALL( // TODO: remove this `ON_FIRST_CALL` block when possible
		const UniformUpdaterParams typed_params = *(UniformUpdaterParams*) param;
		const GLuint shader = drawable -> shader;

		use_texture_in_shader(typed_params.skybox -> diffuse_texture, shader, "environment_map_sampler", TexSkybox, TU_Skybox);
		use_texture_in_shader(typed_params.sector_context -> drawable.diffuse_texture, shader, "diffuse_sampler", TexSet, TU_SectorFaceDiffuse);
		use_texture_in_shader(typed_params.sector_context -> normal_map_set, shader, "normal_map_sampler", TexSet, TU_SectorFaceNormalMap);
		use_texture_in_shader(typed_params.shadow_context -> depth_layers, shader, "shadow_cascade_sampler", shadow_map_texture_type, TU_CascadedShadowMap);
		use_texture_in_shader(typed_params.ao_map, shader, "ambient_occlusion_sampler", TexVolumetric, TU_AmbientOcclusionMap);
	);
}

static void define_vertex_spec(void) {
	enum {vpt = vertices_per_triangle};
	const GLenum typename = FACE_COMPONENT_TYPENAME;

	define_vertex_spec_index(false, true, 0, vpt, sizeof(face_vertex_t), 0, typename); // Position
	define_vertex_spec_index(false, false, 1, 1, sizeof(face_vertex_t), sizeof(face_component_t[vpt]), typename); // Face info
}

////////// Initialization, deinitialization, and rendering

SectorContext init_sector_context(const byte* const heightmap,
	const byte* const texture_id_map, const byte map_width, const byte map_height,
	const GLchar* const* const texture_paths, const GLsizei num_textures,
	const GLsizei texture_size, const NormalMapConfig* const normal_map_config) {

	List sectors, face_meshes;
	generate_sectors_and_face_meshes_from_maps(&sectors, &face_meshes, heightmap, texture_id_map, map_width, map_height);

	const GLuint diffuse_texture_set = init_texture_set(
		false, TexRepeating, OPENGL_SCENE_MAG_FILTER, OPENGL_SCENE_MIN_FILTER,
		num_textures, 0, texture_size, texture_size, texture_paths, NULL
	);

	return (SectorContext) {
		.drawable = init_drawable_with_vertices(
			define_vertex_spec, (uniform_updater_t) update_uniforms, GL_DYNAMIC_DRAW, GL_TRIANGLES,
			(List) {.data = NULL, .item_size = face_meshes.item_size, .length = face_meshes.length},
			init_shader(ASSET_PATH("shaders/sector.vert"), NULL, ASSET_PATH("shaders/sector.frag"), NULL),
			diffuse_texture_set
		),

		.normal_map_set = init_normal_map_from_diffuse_texture(diffuse_texture_set, TexSet, normal_map_config),
		.mesh_cpu = face_meshes, .sectors = sectors
	};
}

void deinit_sector_context(const SectorContext* const sector_context) {
	deinit_drawable(sector_context -> drawable);
	deinit_texture(sector_context -> normal_map_set);
	deinit_list(sector_context -> mesh_cpu);
	deinit_list(sector_context -> sectors);
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
			&(UniformUpdaterParams) {skybox, sector_context, shadow_context, ao_map},
			UseShaderPipeline | BindVertexSpec);
}
