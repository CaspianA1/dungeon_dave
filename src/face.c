#ifndef FACE_C
#define FACE_C

#include "face.h"

/*
static void print_face(const Face face, const GLchar* const prefix_msg) {
	const GLchar* const type_string =
		(GLchar*[]) {"Flat", "Vert_NS", "Vert_EW"}[face.type];

	printf("%s{.type = %s, .origin = {%d, %d}, .size = {%d, %d}}\n",
		prefix_msg, type_string, face.origin[0],
		face.origin[1], face.size[0], face.size[1]);
}
*/

// Returns if there is another face to get
static bool get_next_face(const Sector sector, const byte varying_axis,
	const byte adjacent_side_val, const byte map_width,
	const byte* const heightmap, Face* const face) {
	
	const byte end_edge_val = sector.origin[varying_axis] + sector.size[varying_axis];

	byte face_height_diff = 0, start_val = face -> origin[varying_axis] + face -> size[0], map_point_params[2];
	map_point_params[!varying_axis] = adjacent_side_val;

	while (start_val < end_edge_val) {
		map_point_params[varying_axis] = start_val;
		const int16_t height_diff = sector.visible_heights.max - sample_map_point(
			heightmap, map_point_params[0], map_point_params[1], map_width);

		if (height_diff > 0) {
			face_height_diff = (byte) height_diff;
			break;
		}
		start_val++;
	}

	if (start_val == end_edge_val) return false;

	byte end_val = start_val;
	while (end_val < end_edge_val) {
		map_point_params[varying_axis] = end_val;
		const int16_t height_diff = sector.visible_heights.max - sample_map_point(
			heightmap, map_point_params[0], map_point_params[1], map_width);

		if (height_diff != face_height_diff) break;
		end_val++;
	}

	face -> origin[varying_axis] = start_val;
	face -> size[0] = end_val - start_val;
	face -> size[1] = face_height_diff;

	return true;
}

static void add_face_mesh_to_list(const Face face, const byte sector_max_visible_height,
	const byte side, const byte texture_id, List* const face_mesh_list) {

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

	const byte
		face_info = (byte) (texture_id << 3u) | face_id,
		near_x = face.origin[0], near_z = face.origin[1],
		top_y = sector_max_visible_height;

	switch (face.type) {
		case Flat: {
			const byte size_x = face.size[0], size_z = face.size[1];
			const byte far_x = near_x + size_x, far_z = near_z + size_z;

			push_ptr_to_list(face_mesh_list,
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
			const byte size_z = face.size[0], size_y = face.size[1];
			const byte far_z = near_z + size_z, bottom_y = top_y - size_y;

			push_ptr_to_list(face_mesh_list,
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
			const byte size_x = face.size[0], size_y = face.size[1];
			const byte far_x = near_x + size_x, bottom_y = top_y - size_y;

			push_ptr_to_list(face_mesh_list,
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

static void init_vert_faces(
	const Sector sector, List* const face_mesh_list,
	const byte* const heightmap, const byte map_width,
	const byte map_height, byte* const biggest_face_height) {

	const byte dimensions[2] = {map_width, map_height};

	for (byte unvarying_axis = 0; unvarying_axis < 2; unvarying_axis++) {
		for (byte side = 0; side < 2; side++) {

			Face next_face = {
				.type = Vert_NS + unvarying_axis,
				.origin = {sector.origin[0], sector.origin[1]}
			};

			byte adjacent_side_val;

			if (side) { // `side` is a top or left of the top-down sector
				const byte unvarying_axis_origin = next_face.origin[unvarying_axis];
				if (unvarying_axis_origin == 0) continue;
				adjacent_side_val = unvarying_axis_origin - 1;
			}
			else {
				byte* const unvarying_axis_origin = next_face.origin + unvarying_axis;
				if ((*unvarying_axis_origin += sector.size[unvarying_axis]) == dimensions[unvarying_axis]) continue;
				adjacent_side_val = *unvarying_axis_origin;
			}

			while (get_next_face(sector, !unvarying_axis, adjacent_side_val, map_width, heightmap, &next_face)) {
				add_face_mesh_to_list(next_face, sector.visible_heights.max, side, sector.texture_id, face_mesh_list);

				const byte face_height = next_face.size[1];
				if (face_height > *biggest_face_height) *biggest_face_height = face_height;
			}
		}
	}
}

List init_face_meshes_from_sectors(const List* const sectors,
	const byte* const heightmap, const byte map_width, const byte map_height) {

	const buffer_size_t num_sectors = sectors -> length;

	/* This contains the actual data for faces. `num_sectors * 3` gives a good
	guess for the face/sector ratio. TODO: make this a constant somewhere. */
	List face_meshes = init_list(num_sectors * 3, face_mesh_t);

	LIST_FOR_EACH(0, sectors, untyped_sector_ref, _,
		Sector* const sector_ref = (Sector*) untyped_sector_ref;
		sector_ref -> face_range.start = face_meshes.length;

		const Sector sector = *sector_ref;
		const Face flat_face = {Flat, {sector.origin[0], sector.origin[1]}, {sector.size[0], sector.size[1]}};
		add_face_mesh_to_list(flat_face, sector.visible_heights.max, 0, sector.texture_id, &face_meshes);

		byte biggest_face_height = 0;
		init_vert_faces(sector, &face_meshes, heightmap, map_width, map_height, &biggest_face_height);

		sector_ref -> visible_heights.min = sector.visible_heights.max - biggest_face_height;
		sector_ref -> face_range.length = face_meshes.length - sector_ref -> face_range.start;
	);

	return face_meshes;
}

#endif
