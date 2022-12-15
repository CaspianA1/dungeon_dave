#include "rendering/entities/sector.h"
#include "utils/map_utils.h" // For `sample_map_point`
#include "utils/bitarray.h" // // For various `BitArray`-related defs
#include "rendering/entities/face.h" // For `init_mesh_for_sector`, and `init_map_edge_mesh`
#include "utils/opengl_wrappers.h" // For various OpenGL wrappers
#include "utils/macro_utils.h" // For `ARRAY_LENGTH`, and `ASSET_PATH`
#include "utils/shader.h" // For `init_shader`

/* TODO:
- For dynamic sectors, perhaps have a 2D floating-point map that represents the displacement
height of vertices, so that sectors can be pulled up or down from the ground

- Fix the bug where pushing oneself in a corner, and looking up and down with the right yaw clips a whole sector face
- To fix this whole depth clamping mess,
	1. Use a different projection matrix for the weapon with a much nearer clip dist
	2. Choose a reasonably near clip dist for the scene
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

// Attributes here are height and texture id
static byte point_matches_sector_attributes(const Sector* const sector,
	const byte* const heightmap, const byte* const texture_id_map,
	const byte x, const byte y, const byte map_width, const byte texture_id) {

	return
		sample_map_point(heightmap, x, y, map_width) == sector -> visible_heights.max &&
		sample_map_point(texture_id_map, x, y, map_width) == texture_id;
}

// Gets length across, and then adds to area size y until out of map or length across not eq
static void form_sector_area(Sector* const sector, const BitArray traversed_points,
	const byte* const heightmap, const byte* const texture_id_map,
	const byte map_width, const byte map_height, const byte texture_id) {

	const byte* const origin = sector -> origin;
	byte* const size = sector -> size;

	byte top_x = origin[0];
	const byte origin_y = origin[1];

	while (top_x < map_width && !bitarray_bit_is_set(traversed_points, origin_y * map_width + top_x) &&
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

	done: {
		const byte base_x_index_for_map = origin[0], max_x_index_for_map = size[0] - 1;

		for (buffer_size_t y = origin_y; y < origin_y + size[1]; y++) {
			const buffer_size_t traversed_points_base_index = y * map_width + base_x_index_for_map;

			set_bit_range_in_bitarray(traversed_points,
				traversed_points_base_index,
				traversed_points_base_index + max_x_index_for_map
			);
		}
	}
}

static void generate_sectors_and_face_mesh_from_maps(List* const sectors, List* const face_mesh,
	const byte* const heightmap, const byte* const texture_id_map, const byte map_width, const byte map_height) {

	/* `>> 3` = `/ 8`. Works pretty well for my maps. TODO: make to a constant.
	If the map size is small enough, an incorrect guess of zero can happen. */
	buffer_size_t sector_amount_guess = (map_width * map_height) >> 3;
	if (sector_amount_guess == 0) sector_amount_guess = 1;

	*sectors = init_list(sector_amount_guess, Sector);

	//////////

	/* This contains the actual data for faces. `num_sectors * 3` gives a good
	guess for the face/sector ratio. TODO: make this a constant somewhere. */
	*face_mesh = init_list(sector_amount_guess * 3, face_mesh_t);

	//////////

	/* This is used to keep track of traversed points. To perform some action on a traversed point
	at position <x, y>, the bit index will be `y * map_width + x`. */
	const BitArray traversed_points = init_bitarray(map_width * map_height);

	for (byte y = 0; y < map_height; y++) {
		const buffer_size_t traversed_points_base_index = y * map_width;
		for (byte x = 0; x < map_width; x++) {
			if (bitarray_bit_is_set(traversed_points, traversed_points_base_index + x)) continue;

			////////// Finding a texture id

			const byte texture_id = sample_map_point(texture_id_map, x, y, map_width);

			if (texture_id >= MAX_NUM_SECTOR_SUBTEXTURES)
				FAIL(InvalidTextureID, "Could not create a sector at map position {%hhu, %hhu} because the texture "
					"ID %hhu exceeds the maximum, which is %hhu", x, y, texture_id, (byte) (MAX_NUM_SECTOR_SUBTEXTURES - 1u));

			////////// Forming a sector

			Sector sector = {
				.origin = {x, y}, .size = {0, 0},
				.visible_heights = {.min = 0, .max = sample_map_point(heightmap, x, y, map_width)},
				.face_range = {.start = 0, .length = 0}
			};

			form_sector_area(&sector, traversed_points, heightmap, texture_id_map, map_width, map_height, texture_id);

			////////// Setting face mesh metadata + initing sector faces

			sector.face_range.start = face_mesh -> length;

			byte biggest_face_height;
			init_mesh_for_sector(&sector, face_mesh, &biggest_face_height, heightmap, map_width, map_height, texture_id);

			sector.visible_heights.min = sector.visible_heights.max - biggest_face_height;
			sector.face_range.length = face_mesh -> length - sector.face_range.start;
			push_ptr_to_list(sectors, &sector);

			//////////

			/* This is a simple optimization; the next `sector_width - 1`
			tiles will already be marked traversed, so this just skips those. */
			x += sector.size[0] - 1;
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

static void init_trimmed_face_mesh_for_shadow_mapping(
	const byte map_width, const byte map_height, const byte* const heightmap,
	const List* const face_mesh, const DynamicLightConfig* const dynamic_light_config,
	GLsizei* const num_vertices, GLuint* const vertex_buffer, GLuint* const vertex_spec) {

	////////// Getting the dynamic light dirs

	// TODO: avoid repeating this logic in `init_dynamic_light`
	vec3 light_dir_from, light_dir_to;
	const GLfloat* const dynamic_light_pos = dynamic_light_config -> pos;

	glm_vec3_sub((GLfloat*) dynamic_light_pos, (GLfloat*) dynamic_light_config -> looking_at.origin, light_dir_from);
	glm_vec3_sub((GLfloat*) dynamic_light_pos, (GLfloat*) dynamic_light_config -> looking_at.dest, light_dir_to);

	glm_vec3_normalize(light_dir_from);
	glm_vec3_normalize(light_dir_to);

	////////// Making a map edge mesh + vertex buffer for it

	List map_edge_mesh = init_map_edge_mesh(heightmap, map_width, map_height);
	const buffer_size_t initial_num_vertices = vertices_per_face * (face_mesh -> length + map_edge_mesh.length);

	////////// Allocing a buffer to write the trimmed face mesh into

	// This is a trimmed vertex because it has no face info bits
	typedef face_component_t trimmed_face_vertex_t[components_per_face_vertex_pos];

	// Note: not using the map edge mesh buffer for this, because that would require in-place shifting of face meshes
	face_component_t* const trimmed_vertices_cpu = alloc(initial_num_vertices, sizeof(trimmed_face_vertex_t));
	face_component_t* trimmed_vertices_cpu_end = trimmed_vertices_cpu;

	//////////

	const List* const face_mesh_lists[2] = {face_mesh, &map_edge_mesh};

	for (byte i = 0; i < ARRAY_LENGTH(face_mesh_lists); i++) {
		const List* const face_mesh_list = face_mesh_lists[i];

		LIST_FOR_EACH(face_mesh_list, face_mesh_t, face_submesh,
			////////// Figuring out if the face is frontfacing for either light direction

			const face_vertex_t* const vertices = (face_vertex_t*) face_submesh;
			const face_component_t *const v0 = vertices[0], *const v1 = vertices[1], *const v2 = vertices[2];

			const int8_t
				sign_x = (int8_t) glm_sign(v0[0] - v1[0]),
				sign_y = (int8_t) glm_sign(v2[1] - v0[1]),
				sign_z = (int8_t) glm_sign(v0[2] - v1[2]);

			// Created by reducing a solution involving the cross product
			const vec3 normal = {sign_y * sign_z, !sign_y, sign_x & sign_y};

			//////////

			/* While the zero-height floor could be removed from the shadow map,
			it's kept because removing it interferes with ESM filtering */

			// If the face is frontfacing for any light dir, keep it
			if (glm_vec3_dot(light_dir_from, (GLfloat*) normal) > 0.0f ||
				glm_vec3_dot(light_dir_to, (GLfloat*) normal) > 0.0f) {

				for (byte j = 0; j < vertices_per_face; j++) {
					const face_component_t* const vertex = ((face_vertex_t*) face_submesh)[j];

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
	const GLsizeiptr num_bytes = num_components * (GLsizeiptr) sizeof(face_component_t);

	*num_vertices = num_components / components_per_face_vertex_pos;

	use_vertex_buffer(*vertex_buffer = init_gpu_buffer());
	init_vertex_buffer_data(num_bytes, 1, trimmed_vertices_cpu, GL_STATIC_DRAW);

	dealloc(trimmed_vertices_cpu);

	use_vertex_spec(*vertex_spec = init_vertex_spec());
	define_vertex_spec_index(false, false, 0, components_per_face_vertex_pos, 0, 0, FACE_COMPONENT_TYPENAME);
}

// The output variable indices are in regards to the `glDrawArrays` call.
static void frustum_cull_sector_faces_into_gpu_buffer(
	const SectorContext* const sector_context, const Camera* const camera,
	buffer_size_t* const first_face_index_ref, buffer_size_t* const num_visible_faces_ref) {

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
			const byte *const origin = sector -> origin, *const size = sector -> size;
			const byte origin_x = origin[0], origin_z = origin[1];

			const vec3 aabb[2] = {
				{origin_x, sector -> visible_heights.min, origin_z},
				{origin_x + size[0], sector -> visible_heights.max, origin_z + size[1]}
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
	define_vertex_spec_index(false, false, 0, components_per_face_vertex_pos, sizeof(face_vertex_t), 0, FACE_COMPONENT_TYPENAME); // Pos

	define_vertex_spec_index(false, false, 1, 1, sizeof(face_vertex_t), // Face info
		sizeof(face_component_t[components_per_face_vertex_pos]), FACE_COMPONENT_TYPENAME);
}

////////// Initialization, deinitialization, and rendering

SectorContext init_sector_context(const byte* const heightmap,
	const byte* const texture_id_map, const byte map_width, const byte map_height,
	const GLchar* const* const texture_paths, const byte num_textures,
	const MaterialPropertiesPerObjectType* const shared_material_properties,
	const DynamicLightConfig* const dynamic_light_config) {

	List sectors, face_mesh;
	generate_sectors_and_face_mesh_from_maps(&sectors, &face_mesh, heightmap, texture_id_map, map_width, map_height);

	//////////

	GLsizei num_vertices_for_shadow_mapping;
	GLuint vertex_buffer_for_shadow_mapping, vertex_spec_for_shadow_mapping;

	init_trimmed_face_mesh_for_shadow_mapping(map_width, map_height,
		heightmap, &face_mesh, dynamic_light_config, &num_vertices_for_shadow_mapping,
		&vertex_buffer_for_shadow_mapping, &vertex_spec_for_shadow_mapping);

	//////////

	const GLsizei texture_size = shared_material_properties -> texture_rescale_size;

	const GLuint albedo_texture_set = init_texture_set(
		false, TexRepeating, OPENGL_SCENE_MAG_FILTER, OPENGL_SCENE_MIN_FILTER,
		num_textures, 0, texture_size, texture_size, texture_paths, NULL
	);

	return (SectorContext) {
		.drawable = init_drawable_with_vertices(
			define_vertex_spec, NULL, GL_DYNAMIC_DRAW, GL_TRIANGLES,
			(List) {.data = NULL, .item_size = face_mesh.item_size, .length = face_mesh.length},
			init_shader(ASSET_PATH("shaders/sector.vert"), NULL, ASSET_PATH("shaders/world_shaded_object.frag"), NULL),
			albedo_texture_set, init_normal_map_from_albedo_texture(
				albedo_texture_set, TexSet, &shared_material_properties -> normal_map_config
			)
		),

		.shadow_mapping = {
			.num_vertices = num_vertices_for_shadow_mapping,

			.vertex_buffer = vertex_buffer_for_shadow_mapping,
			.vertex_spec = vertex_spec_for_shadow_mapping,

			.depth_shader = init_shader(
				ASSET_PATH("shaders/shadow/sector_depth.vert"),
				ASSET_PATH("shaders/shadow/sector_depth.geom"),
				ASSET_PATH("shaders/shadow/sector_depth.frag"),
				NULL
			)
		},

		.mesh_cpu = face_mesh, .sectors = sectors
	};
}

void deinit_sector_context(const SectorContext* const sector_context) {
	deinit_drawable(sector_context -> drawable);

	deinit_gpu_buffer(sector_context -> shadow_mapping.vertex_buffer);
	deinit_vertex_spec(sector_context -> shadow_mapping.vertex_spec);
	deinit_shader(sector_context -> shadow_mapping.depth_shader);

	deinit_list(sector_context -> mesh_cpu);
	deinit_list(sector_context -> sectors);
}

void draw_sectors_to_shadow_context(const SectorContext* const sector_context) {
	use_shader(sector_context -> shadow_mapping.depth_shader);
	use_vertex_spec(sector_context -> shadow_mapping.vertex_spec);
	draw_primitives(sector_context -> drawable.triangle_mode, sector_context -> shadow_mapping.num_vertices);
}

void draw_sectors(const SectorContext* const sector_context, const Camera* const camera) {
	buffer_size_t first_face_index, num_visible_faces;
	frustum_cull_sector_faces_into_gpu_buffer(sector_context, camera, &first_face_index, &num_visible_faces);

	// If looking out at the distance with no sectors, why do any state switching at all?
	if (num_visible_faces != 0) {
		// TODO: call `draw_drawable` here instead
		const Drawable* const drawable = &sector_context -> drawable;

		const GLint start_vertex = (GLint) (first_face_index * vertices_per_face);
		const GLsizei num_vertices = (GLsizei) (num_visible_faces * vertices_per_face);

		use_vertex_spec(drawable -> vertex_spec);
		use_shader(drawable -> shader);
		glDrawArrays(GL_TRIANGLES, start_vertex, num_vertices);
	}
}
