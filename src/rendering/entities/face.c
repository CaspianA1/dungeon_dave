#include "rendering/entities/face.h"
#include "utils/map_utils.h" // For `sample_map`, `get_indexed_map_pos_component`, and `get_indexed_map_pos_component_ref`

/* For the face type, NS = north-south, and EW = east-west.
If a face is NS, its two ends lie on a vertical top-down axis;
and if a face is EW, its two ends lie on a horizontal top-down axis.

Faces don't store their height origin, since sectors store that.
For vert faces, origin and size[0] are top-down, and size[1] is depth.
For hori faces, origin and size are both top-down. */

typedef struct {
	const enum {Flat, Vert_NS, Vert_EW} type;
	map_pos_component_t origin[2], size[2];
} Face;

//////////

/*
static void print_face(const Face face, const GLchar* const prefix_msg) {
	const GLchar* const type_string = (GLchar*[]) {"Flat", "Vert_NS", "Vert_EW"}[face.type];

	printf("%s{.type = %s, .origin = {%u, %u}, .size = {%u, %u}}\n",
		prefix_msg, type_string, face.origin[0],
		face.origin[1], face.size[0], face.size[1]);
}
*/

// Returns if there is another face to get. Initializes the face origin on the varying axis, and the face's size too.
static bool get_next_face(const byte varying_axis, const map_pos_component_t adjacent_side_val,
	const Heightmap heightmap, const Sector* const sector, Face* const face) {

	map_pos_component_t
		*const face_size = face -> size,
		*const face_origin_on_varying_axis = face -> origin + varying_axis;

	const map_pos_component_t
		max_visible_height = sector -> visible_heights.max,
		end_edge_val = get_indexed_map_pos_component(sector -> origin, varying_axis)
					+ get_indexed_map_pos_component(sector -> size, varying_axis);

	map_pos_component_t face_height_diff = 0, start_val = *face_origin_on_varying_axis + face_size[0];

	//////////

	map_pos_xz_t map_point_params;
	*get_indexed_map_pos_component_ref(&map_point_params, !varying_axis) = adjacent_side_val;
	map_pos_component_t* const varying_map_point_value = get_indexed_map_pos_component_ref(&map_point_params, varying_axis);

	//////////

	while (start_val < end_edge_val) {
		*varying_map_point_value = start_val;
		const signed_map_pos_component_t height_diff = max_visible_height - sample_map(heightmap, map_point_params);

		if (height_diff > 0) {
			face_height_diff = (map_pos_component_t) height_diff;
			break;
		}
		start_val++;
	}

	if (start_val == end_edge_val) return false;

	map_pos_component_t end_val = start_val;
	while (end_val < end_edge_val) {
		*varying_map_point_value = end_val;
		const signed_map_pos_component_t height_diff = max_visible_height - sample_map(heightmap, map_point_params);

		if (height_diff != face_height_diff) break;
		end_val++;
	}

	*face_origin_on_varying_axis = start_val;
	face_size[0] = end_val - start_val;
	face_size[1] = face_height_diff;

	return true;
}

static void add_to_face_mesh(List* const face_mesh,
	const Face face, const map_pos_component_t sector_max_visible_height,
	const byte side, const map_texture_id_t texture_id) {

	/* Face info bits, layout:
	Bits 0-2, three bits -> face id.
		0 = flat,
		1 = right vert NS,
		2 = bottom vert EW,
		3 = left vert NS,
		4 = top vert EW.
	Bits 3-7, five bits -> texture id. */

	// `u` suffixes used to reduce the chance of undefined behavior with signed bitwise operations
	byte face_id = (byte) (side << 2u) | (byte) face.type;
	if (face_id == 5u || face_id == 6u) face_id -= 2u;

	const byte face_info = (byte) (texture_id << 3u) | face_id;

	const map_pos_component_t
		near_x = face.origin[0], near_z = face.origin[1],
		top_y = sector_max_visible_height;

	switch (face.type) {
		case Flat: {
			const map_pos_component_t size_x = face.size[0], size_z = face.size[1];
			const map_pos_component_t far_x = near_x + size_x, far_z = near_z + size_z;

			push_ptr_to_list(face_mesh,
				(face_mesh_t) {
					{near_x, top_y, far_z, face_info},
					{far_x, top_y, near_z, face_info},
					{near_x, top_y, near_z, face_info},
					{near_x, top_y, far_z, face_info},
					{far_x, top_y, far_z, face_info},
					{far_x, top_y, near_z, face_info}
				});

			break;
		}
		case Vert_NS: {
			const map_pos_component_t size_z = face.size[0], size_y = face.size[1];
			const map_pos_component_t far_z = near_z + size_z, bottom_y = top_y - size_y;

			push_ptr_to_list(face_mesh,
				side ? (face_mesh_t) {
					{near_x, bottom_y, near_z, face_info},
					{near_x, top_y, far_z, face_info},
					{near_x, top_y, near_z, face_info},
					{near_x, bottom_y, near_z, face_info},
					{near_x, bottom_y, far_z, face_info},
					{near_x, top_y, far_z, face_info}
				} : (face_mesh_t) {
					{near_x, top_y, near_z, face_info},
					{near_x, top_y, far_z, face_info},
					{near_x, bottom_y, near_z, face_info},
					{near_x, top_y, far_z, face_info},
					{near_x, bottom_y, far_z, face_info},
					{near_x, bottom_y, near_z, face_info}
				});

			break;
		}
		case Vert_EW: {
			const map_pos_component_t size_x = face.size[0], size_y = face.size[1];
			const map_pos_component_t far_x = near_x + size_x, bottom_y = top_y - size_y;

			push_ptr_to_list(face_mesh,
				side ? (face_mesh_t) {
					{near_x, top_y, near_z, face_info},
					{far_x, top_y, near_z, face_info},
					{near_x, bottom_y, near_z, face_info},
					{far_x, top_y, near_z, face_info},
					{far_x, bottom_y, near_z, face_info},
					{near_x, bottom_y, near_z, face_info}
				} : (face_mesh_t) {
					{near_x, bottom_y, near_z, face_info},
					{far_x, top_y, near_z, face_info},
					{near_x, top_y, near_z, face_info},
					{near_x, bottom_y, near_z, face_info},
					{far_x, bottom_y, near_z, face_info},
					{far_x, top_y, near_z, face_info}
				});

			break;
		}
	}
}

// This initializes `face_mesh` and `biggest_face_height`
void init_mesh_for_sector(const Sector* const sector,
	List* const face_mesh, map_pos_component_t* const biggest_face_height,
	const Heightmap heightmap, const map_texture_id_t texture_id) {

	const map_pos_component_t max_visible_height = sector -> visible_heights.max;
	const map_pos_xz_t origin_xz = sector -> origin, size_xz = sector -> size;

	add_to_face_mesh(face_mesh, // Adding the flat face
		(Face) {Flat, {origin_xz.x, origin_xz.z}, {size_xz.x, size_xz.z}},
		max_visible_height, 0, texture_id);

	//////////

	*biggest_face_height = 0;

	for (byte unvarying_axis = 0; unvarying_axis < 2; unvarying_axis++) {
		const map_pos_component_t
			unvarying_axis_map_size = get_indexed_map_pos_component(heightmap.size, unvarying_axis),
			unvarying_axis_sector_size = get_indexed_map_pos_component(size_xz, unvarying_axis);

		for (byte side = 0; side < 2; side++) {
			Face next_face = {
				.type = Vert_NS + unvarying_axis,
				.origin = {origin_xz.x, origin_xz.z}
			};

			map_pos_component_t adjacent_side_val;

			if (side) { // `side` is a top or left of the top-down sector
				const map_pos_component_t unvarying_axis_origin = next_face.origin[unvarying_axis];
				if (unvarying_axis_origin == 0) continue;
				else adjacent_side_val = unvarying_axis_origin - 1;
			}
			else {
				map_pos_component_t* const unvarying_axis_origin = next_face.origin + unvarying_axis;
				if ((*unvarying_axis_origin += unvarying_axis_sector_size) == unvarying_axis_map_size) continue;
				else adjacent_side_val = *unvarying_axis_origin;
			}

			while (get_next_face(!unvarying_axis, adjacent_side_val, heightmap, sector, &next_face)) {
				add_to_face_mesh(face_mesh, next_face, max_visible_height, side, texture_id);

				const map_pos_component_t face_height = next_face.size[1];
				if (face_height > *biggest_face_height) *biggest_face_height = face_height;
			}
		}
	}
}

List init_map_edge_mesh(const Heightmap heightmap) {
	// TODO: make this a constant somewhere
	buffer_size_t submesh_amount_guess = (heightmap.size.x + heightmap.size.z) / 6;
	if (submesh_amount_guess == 0) submesh_amount_guess = 1;

	List edge_mesh = init_list(submesh_amount_guess, face_mesh_t);

	//////////

	for (byte unvarying_axis = 0; unvarying_axis < 2; unvarying_axis++) {
		Face face = {.type = Vert_NS + unvarying_axis, .size = {0, 0}};

		for (byte side = 0; side < 2; side++) { // `side` == top or left
			const byte varying_axis = !unvarying_axis;

			face.origin[varying_axis] = 0;
			face.origin[unvarying_axis] = side ? 0 : (get_indexed_map_pos_component(heightmap.size, unvarying_axis) - 1);

			//////////

			bool building_edge_faces = true;

			while (building_edge_faces) {
				const map_pos_component_t face_height = sample_map(heightmap, (map_pos_xz_t) {face.origin[0], face.origin[1]});

				map_pos_xz_t curr_map_pos = {face.origin[0], face.origin[1]};
				map_pos_component_t* const varying_bottom_of_edge = get_indexed_map_pos_component_ref(&curr_map_pos, varying_axis);
				const map_pos_component_t varying_map_boundary = get_indexed_map_pos_component(heightmap.size, varying_axis);

				while (building_edge_faces && sample_map(heightmap, curr_map_pos) == face_height) {
					(*varying_bottom_of_edge)++;
					building_edge_faces = (*varying_bottom_of_edge < varying_map_boundary);
				}

				face.size[0] = *varying_bottom_of_edge - face.origin[varying_axis];
				face.size[1] = face_height;

				//////////

				if (face_height != 0) { // No face mesh generated for height-zero faces
					map_pos_component_t* const unvarying_origin = face.origin + unvarying_axis;

					if (!side) (*unvarying_origin)++; // Putting the face on the other block side
					add_to_face_mesh(&edge_mesh, face, face_height, side, 0);
					if (!side) (*unvarying_origin)--;
				}

				face.origin[varying_axis] += face.size[0]; // Extending the origin by its size
				face.size[0] = 0; // Resetting the size
			}
		}
	}

	return edge_mesh;
}
