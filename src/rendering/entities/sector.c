#include "rendering/entities/sector.h"
#include "utils/map_utils.h" // For `sample_map`
#include "utils/bitarray.h" // // For various `BitArray`-related defs
#include "rendering/entities/face.h" // For `init_mesh_for_sector`, and `init_map_edge_mesh`
#include "utils/opengl_wrappers.h" // For various OpenGL wrappers
#include "utils/macro_utils.h" // For `ARRAY_LENGTH`
#include "utils/shader.h" // For `init_shader`
#include "data/constants.h" // For `default_depth_func`

/* TODO:
- For dynamic sectors, perhaps have a 2D floating-point map that represents the displacement
height of vertices, so that sectors can be pulled up or down from the ground

- Fix the bug where pushing oneself in a corner, and looking up and down with the right yaw clips a whole sector face
- To fix this whole depth clamping mess,
	1. Use a different projection matrix for the weapon with a much nearer clip dist
	2. Choose a reasonably near clip dist for the level
	3. Disable depth clamping

Further sector mesh optimization for shadow mapping:
	- Pre-cull backfacing faces, based on the light direction - done
	- Perhaps also cull sectors if they don't cast shadows (will that interfere with ESM filtering?)
	- Merge faces with the same texture id (not that big of an advantage to that, really)
	- Sorting from front to back
	- Indexing, if it reduces the total size at all
A non shadow mapping optimization:
	- For frustum visibility checks, merge adjacent sectors that have the same texture id
	- So then, many sub-sub-face meshes may belong to a single sector
*/

////////// Constants

/* There's five bits to store a texture id in a face mesh's face info byte,
And the biggest number possible with five bits is 31, so that gives you
32 different possible texture ids. Also, this is just for wall textures. */
const map_texture_id_t max_num_sector_subtextures = 32u;

////////// The sector generation code

// Attributes here are height and texture id
static byte point_matches_sector_attributes(const Sector* const sector,
	const Heightmap heightmap, const map_texture_id_t* const texture_id_map_data,
	const map_pos_xz_t pos, const map_texture_id_t texture_id) {

	return
		sample_map(heightmap, pos) == sector -> visible_heights.max &&
		sample_texture_id_map(texture_id_map_data, heightmap.size.x, pos) == texture_id;
}

// Gets length across, and then adds to area size z until out of map or length across not eq
static void form_sector_area(Sector* const sector, const BitArray traversed_points,
	const Heightmap heightmap, const map_texture_id_t* const texture_id_map_data,
	const map_texture_id_t texture_id) {

	const map_pos_xz_t origin = sector -> origin;

	map_pos_xz_t size = sector -> size;
	map_pos_component_t top_x = origin.x;

	while (top_x < heightmap.size.x && !bitarray_bit_is_set(traversed_points, origin.z * heightmap.size.x + top_x) &&
		point_matches_sector_attributes(sector, heightmap, texture_id_map_data, (map_pos_xz_t) {top_x, origin.z}, texture_id)) {

		size.x++;
		top_x++;
	}

	// Now, `sector.size.x` equals the first horizontal span length where equal attributes were found
	for (map_pos_component_t z = origin.z; z < heightmap.size.z; z++, size.z++) {
		for (map_pos_component_t x = origin.x; x < top_x; x++) { // Here, `top_x` equals the top right corner
			if (!point_matches_sector_attributes(sector, heightmap, texture_id_map_data, (map_pos_xz_t) {x, z}, texture_id))
				goto done;
		}
	}

	done: {
		const map_pos_component_t base_x_index_for_map = origin.x, max_x_index_for_map = size.x - 1;

		for (buffer_size_t z = origin.z; z < origin.z + size.z; z++) {
			const buffer_size_t traversed_points_base_index = z * heightmap.size.x + base_x_index_for_map;

			set_bit_range_in_bitarray(traversed_points,
				traversed_points_base_index,
				traversed_points_base_index + max_x_index_for_map
			);
		}
		sector -> size = size;
	}
}

static void generate_sectors_and_face_mesh_from_maps(List* const sectors, List* const face_mesh,
	const Heightmap heightmap, const map_texture_id_t* const texture_id_map_data) {

	//////////

	const buffer_size_t num_map_grid_values = heightmap.size.x * heightmap.size.z;

	/* `>> 3` = `/ 8`. Works pretty well for my maps. TODO: make to a constant.
	If the map size is small enough, an incorrect guess of zero can happen. */
	buffer_size_t sector_amount_guess = num_map_grid_values >> 3;
	if (sector_amount_guess == 0) sector_amount_guess = 1;

	*sectors = init_list(sector_amount_guess, Sector);

	/* This contains the actual data for faces. `num_sectors * 3` gives a good
	guess for the face/sector ratio. TODO: make this a constant somewhere. */
	*face_mesh = init_list(sector_amount_guess * 3, face_mesh_t);

	//////////

	/* This is used to keep track of traversed points. To perform some action on a traversed point
	at position <x, y>, the bit index will be `z * map_width + x`. */
	const BitArray traversed_points = init_bitarray(num_map_grid_values);

	for (map_pos_component_t z = 0; z < heightmap.size.z; z++) {
		const buffer_size_t traversed_points_base_index = z * heightmap.size.x;
		for (map_pos_component_t x = 0; x < heightmap.size.x; x++) {
			if (bitarray_bit_is_set(traversed_points, traversed_points_base_index + x)) continue;

			////////// Finding a texture id

			const map_texture_id_t texture_id = sample_texture_id_map(texture_id_map_data,
				heightmap.size.x, (map_pos_xz_t) {x, z});

			if (texture_id >= max_num_sector_subtextures)
				FAIL(InvalidTextureID, "Could not create a sector at map position {%u, %u} because the texture "
					"ID %u exceeds the maximum, which is %u", x, z, texture_id, max_num_sector_subtextures - 1u);

			////////// Forming a sector

			Sector sector = {
				.origin = {x, z}, .size = {0, 0},
				.visible_heights = {.min = 0, .max = sample_map(heightmap, (map_pos_xz_t) {x, z})},
				.face_range = {.start = 0, .length = 0}
			};

			form_sector_area(&sector, traversed_points, heightmap, texture_id_map_data, texture_id);

			////////// Setting face mesh metadata + initing sector faces

			sector.face_range.start = face_mesh -> length;

			map_pos_component_t biggest_face_height;
			init_mesh_for_sector(&sector, face_mesh, &biggest_face_height, heightmap, texture_id);

			sector.visible_heights.min = sector.visible_heights.max - biggest_face_height;
			sector.face_range.length = face_mesh -> length - sector.face_range.start;
			push_ptr_to_list(sectors, &sector);

			//////////

			/* This is a simple optimization; the next `sector_width - 1`
			tiles will already be marked traversed, so this just skips those. */
			x += sector.size.x - 1;
		}
	}

	deinit_bitarray(traversed_points);
}

/* This function generates a modified version of the plain face mesh used
for rendering sectors. Here's why a separate mesh is used for shadow mapping:

Note that a separate face mesh is used for rendering shadows because the other vertex buffer
used to store the face mesh changes every frame due to frustum culling, so if I were to use that one,
not all necessary shadows would be cast. Also,

1. It includes map edge geometry. Normally, this geometry is not generated
	because the player will never see it, but this adds that geometry in.

2. It trims the face info bits, so that this version of the mesh used for shadow mapping
	will only take up 75% of its needed space.

3. Given the range of possible light directions for the dynamic light,
	it removes backfacing faces ahead of time, to further reduce the buffer size.

4. Since it never changes, it can stay in high-speed memory.
*/
static void init_trimmed_face_mesh_for_shadow_mapping(const Heightmap heightmap,
	const List* const face_mesh, const DynamicLightConfig* const dynamic_light_config,
	GLsizei* const num_vertices, GLuint* const vertex_buffer, GLuint* const vertex_spec) {

	////////// Getting the dynamic light dirs

	// TODO: avoid repeating this logic in `init_dynamic_light`
	vec3 light_dir_from, light_dir_to;
	glm_vec3_normalize_to((GLfloat*) dynamic_light_config -> unnormalized_from, light_dir_from);
	glm_vec3_normalize_to((GLfloat*) dynamic_light_config -> unnormalized_to, light_dir_to);

	////////// Making a map edge mesh + vertex buffer for it

	List map_edge_mesh = init_map_edge_mesh(heightmap);
	const buffer_size_t initial_num_vertices = vertices_per_face * (face_mesh -> length + map_edge_mesh.length);

	////////// Allocing a buffer to write the trimmed face mesh into

	// This is a trimmed vertex because it has no face info bits
	typedef map_pos_component_t trimmed_face_vertex_t[components_per_face_vertex_pos];

	// Note: not using the map edge mesh buffer for this, because that would require in-place shifting of face meshes
	map_pos_component_t* const trimmed_vertices_cpu = alloc(initial_num_vertices, sizeof(trimmed_face_vertex_t));
	map_pos_component_t* trimmed_vertices_cpu_end = trimmed_vertices_cpu;

	//////////

	const List* const face_mesh_lists[2] = {face_mesh, &map_edge_mesh};

	for (byte i = 0; i < ARRAY_LENGTH(face_mesh_lists); i++) {
		const List* const face_mesh_list = face_mesh_lists[i];

		LIST_FOR_EACH(face_mesh_list, face_mesh_t, face_submesh,
			////////// Figuring out if the face is frontfacing for either light direction

			const face_vertex_t* const vertices = (face_vertex_t*) face_submesh;
			const map_pos_component_t *const v0 = vertices[0], *const v1 = vertices[1], *const v2 = vertices[2];

			const signed_byte
				sign_x = (signed_byte) glm_sign(v0[0] - v1[0]),
				sign_y = (signed_byte) glm_sign(v2[1] - v0[1]),
				sign_z = (signed_byte) glm_sign(v0[2] - v1[2]);

			// Created by reducing a solution involving the cross product
			const vec3 normal = {sign_y * sign_z, !sign_y, sign_x & sign_y};

			//////////

			/* While the zero-height floor could be removed from the shadow map,
			it's kept because removing it interferes with ESM filtering */

			// If the face is frontfacing for any light dir, keep it
			if (glm_vec3_dot(light_dir_from, (GLfloat*) normal) > 0.0f ||
				glm_vec3_dot(light_dir_to, (GLfloat*) normal) > 0.0f) {

				for (byte j = 0; j < vertices_per_face; j++) {
					const map_pos_component_t* const vertex = ((face_vertex_t*) face_submesh)[j];

					*(trimmed_vertices_cpu_end++) = vertex[0];
					*(trimmed_vertices_cpu_end++) = vertex[1];
					*(trimmed_vertices_cpu_end++) = vertex[2];
				}
			}
		);
	}

	deinit_list(map_edge_mesh);

	////////// Defining the number of vertices, the vertex buffer, and the vertex spec

	const GLsizei num_components = (GLsizei) (trimmed_vertices_cpu_end - trimmed_vertices_cpu);
	const GLsizeiptr num_bytes = num_components * (GLsizeiptr) sizeof(map_pos_component_t);

	*num_vertices = num_components / components_per_face_vertex_pos;

	use_vertex_buffer(*vertex_buffer = init_gpu_buffer());
	init_vertex_buffer_data(num_bytes, 1, trimmed_vertices_cpu, GL_STATIC_DRAW);

	dealloc(trimmed_vertices_cpu);

	use_vertex_spec(*vertex_spec = init_vertex_spec());
	define_vertex_spec_index(false, false, 0, components_per_face_vertex_pos, 0, 0, MAP_POS_COMPONENT_TYPENAME);
}

// The output variable indices are in regards to the `glDrawArrays` call.
static void frustum_cull_sector_faces_into_gpu_buffer(
	const SectorContext* const sector_context, const Camera* const camera,
	buffer_size_t* const first_face_index_ref, buffer_size_t* const num_visible_faces_ref) {

	/* TODO: perhaps optimize like this:
	- Every frame: For all sectors that are visible, add their sub-meshes to the GPU buffer
	- On the reinit event: Clear all sub-meshes from the GPU buffer
	- That would result in some occasional overdraw, but it would help if the constant rewriting becomes a botteneck */

	const List* const sectors = &sector_context -> sectors;
	const Sector* const out_of_bounds_sector = (Sector*) sectors -> data + sectors -> length;

	const List* const face_mesh_cpu = &sector_context -> mesh_cpu;
	const face_mesh_t* const face_mesh_cpu_data = face_mesh_cpu -> data;
	const buffer_size_t num_total_faces = face_mesh_cpu -> length;

	const GLuint vertex_buffer = sector_context -> drawable.vertex_buffer;

	face_mesh_t* const face_mesh_gpu = init_vertex_buffer_memory_mapping(
		vertex_buffer, num_total_faces * sizeof(face_mesh_t), true
	);

	const vec4* const frustum_planes = camera -> frustum_planes;
	buffer_size_t num_visible_faces = 0;

	/* Sectors are stored first sorted by their X coordinate, and then by their Z coordinate
	(because that's the order of their creation by the heightmap sector mesher). This is then
	the standard order of the sector sub-meshes in the sector face GPU buffer.

	When looking in the negative Z direction, there's a lot of overdraw. To avoid this, when looking
	in that direction, face meshes are copied into the back of the GPU buffer (rather than the front),
	and filled in right-to-left in memory, so that face meshes that have a larger Z coordinate go before those
	with a smaller Z coordinate in the buffer. This leads to less overdraw, and much better performance overall.

	TODO: apply this process to the X-axis too, if possible. */
	const bool order_face_meshes_backwards = camera -> dir[2] < 0.0f;

	LIST_FOR_EACH(sectors, Sector, sector,
		buffer_size_t num_visible_faces_in_group = 0;
		const buffer_size_t cpu_buffer_start_index = sector -> face_range.start;

		// Checking to see how many sectors forward are visible
		while (sector < out_of_bounds_sector) {
			const map_pos_xz_t origin = sector -> origin, size = sector -> size;

			const vec3 aabb[2] = {
				{origin.x, sector -> visible_heights.min, origin.z},
				{origin.x + size.x, sector -> visible_heights.max, origin.z + size.z}
			};

			if (!glm_aabb_frustum((vec3*) aabb, (vec4*) frustum_planes)) break;

			num_visible_faces_in_group += sector++ -> face_range.length;
		}

		if (num_visible_faces_in_group != 0) {
			face_mesh_t* const gpu_buffer_dest = face_mesh_gpu + (order_face_meshes_backwards
				? (num_total_faces - num_visible_faces - num_visible_faces_in_group)
				: num_visible_faces);

			memcpy(gpu_buffer_dest,
				face_mesh_cpu_data + cpu_buffer_start_index,
				num_visible_faces_in_group * sizeof(face_mesh_t));

			num_visible_faces += num_visible_faces_in_group;
		}
	);

	deinit_vertex_buffer_memory_mapping();

	*num_visible_faces_ref = num_visible_faces;
	*first_face_index_ref = order_face_meshes_backwards ? (face_mesh_cpu -> length - num_visible_faces) : 0u;
}

static void define_vertex_spec(void) {
	define_vertex_spec_index(false, false, 0, components_per_face_vertex_pos,
		sizeof(face_vertex_t), 0, MAP_POS_COMPONENT_TYPENAME); // Pos

	define_vertex_spec_index(false, false, 1, 1, sizeof(face_vertex_t), // Face info
		sizeof(map_pos_component_t[components_per_face_vertex_pos]), MAP_POS_COMPONENT_TYPENAME);
}

////////// Initialization, deinitialization, and rendering

SectorContext init_sector_context(
	const Heightmap heightmap, const map_texture_id_t* const texture_id_map_data,
	const GLchar* const* const texture_paths, const texture_id_t num_textures,
	const MaterialPropertiesPerObjectType* const shared_material_properties,
	const DynamicLightConfig* const dynamic_light_config) {

	////////// Checking that there's not too many sector face textures

	// TODO: also check that the max texture id in the heightmap doesn't exceed the maximum from here
	if (num_textures > max_num_sector_subtextures) FAIL(ReadFromJSON, "Number of sector face texture paths "
		"exceeds the maximum (%u > %u)", num_textures, max_num_sector_subtextures);

	////////// Making a list of sectors, and a face mesh

	List sectors, face_mesh;
	generate_sectors_and_face_mesh_from_maps(&sectors, &face_mesh, heightmap, texture_id_map_data);

	////////// Making a trimmed face mesh for shadow mapping

	GLsizei num_vertices_for_shadow_mapping;
	GLuint vertex_buffer_for_shadow_mapping, vertex_spec_for_shadow_mapping;

	init_trimmed_face_mesh_for_shadow_mapping(heightmap, &face_mesh, dynamic_light_config,
		&num_vertices_for_shadow_mapping, &vertex_buffer_for_shadow_mapping, &vertex_spec_for_shadow_mapping);

	////////// Making an albedo texture set

	const GLsizei texture_size = shared_material_properties -> texture_rescale_size;

	const GLuint albedo_texture_set = init_texture_set(
		false, TexRepeating, OPENGL_LEVEL_MAG_FILTER, OPENGL_LEVEL_MIN_FILTER,
		num_textures, 0, texture_size, texture_size, texture_paths, NULL
	);

	////////// Making a sector context

	return (SectorContext) {
		.drawable = init_drawable_with_vertices(
			define_vertex_spec, NULL, GL_DYNAMIC_DRAW, GL_TRIANGLES,
			(List) {.data = NULL, .item_size = face_mesh.item_size, .length = face_mesh.length},
			init_shader("shaders/sector.vert", NULL, "shaders/common/world_shading.frag", NULL),
			albedo_texture_set, init_normal_map_from_albedo_texture(
				albedo_texture_set, TexSet, &shared_material_properties -> normal_map_config
			)
		),

		.shadow_mapping = {
			.num_vertices = num_vertices_for_shadow_mapping,

			.vertex_buffer = vertex_buffer_for_shadow_mapping,
			.vertex_spec = vertex_spec_for_shadow_mapping,

			.depth_shader = init_shader(
				"shaders/shadow/sector_depth.vert",
				"shaders/shadow/sector_depth.geom",
				"shaders/shadow/sector_depth.frag",
				NULL
			)
		},

		.depth_prepass_shader = init_shader(
			"shaders/sector_depth_prepass.vert",
			NULL,
			"shaders/shadow/sector_depth.frag",
			NULL
		),

		.mesh_cpu = face_mesh, .sectors = sectors
	};
}

void deinit_sector_context(const SectorContext* const sector_context) {
	deinit_drawable(sector_context -> drawable);

	deinit_gpu_buffer(sector_context -> shadow_mapping.vertex_buffer);
	deinit_vertex_spec(sector_context -> shadow_mapping.vertex_spec);
	deinit_shader(sector_context -> shadow_mapping.depth_shader);

	deinit_shader(sector_context -> depth_prepass_shader);

	deinit_list(sector_context -> mesh_cpu);
	deinit_list(sector_context -> sectors);
}

void draw_sectors_to_shadow_context(const SectorContext* const sector_context) {
	use_shader(sector_context -> shadow_mapping.depth_shader);
	use_vertex_spec(sector_context -> shadow_mapping.vertex_spec);
	draw_primitives(sector_context -> drawable.triangle_mode, sector_context -> shadow_mapping.num_vertices);
}

void draw_sectors(const SectorContext* const sector_context, const Camera* const camera) {
	/* TODO: use `glMultiDrawArrays` around here instead, to avoid too much CPU -> GPU copying?
	When starting this out, just start with some normal `glDrawArrays` calls. */

	buffer_size_t first_face_index, num_visible_faces;
	frustum_cull_sector_faces_into_gpu_buffer(sector_context, camera, &first_face_index, &num_visible_faces);

	// If looking out at the distance with no sectors, why do any state switching at all?
	if (num_visible_faces != 0) {
		// TODO: call `draw_drawable` here instead
		const Drawable* const drawable = &sector_context -> drawable;

		const GLint start_vertex = (GLint) (first_face_index * vertices_per_face);
		const GLsizei num_vertices = (GLsizei) (num_visible_faces * vertices_per_face);

		use_vertex_spec(drawable -> vertex_spec);

		////////// Depth prepass

		// Running a depth prepass
		use_shader(sector_context -> depth_prepass_shader);

		 // No color buffer writes
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
			glDrawArrays(GL_TRIANGLES, start_vertex, num_vertices);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		////////// Rendering pass

		// Only passing fragments with the same depth value
		WITH_RENDER_STATE(glDepthFunc, GL_EQUAL, constants.default_depth_func,
			// No depth buffer writes (TODO: stop the redundant state change for this in 'main.c')
			WITH_RENDER_STATE(glDepthMask, GL_FALSE, GL_TRUE,
				use_shader(drawable -> shader);
				glDrawArrays(GL_TRIANGLES, start_vertex, num_vertices);
			);
		);
	}
}
