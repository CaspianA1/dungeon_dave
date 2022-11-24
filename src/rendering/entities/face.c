#include "rendering/entities/face.h"
#include "utils/map_utils.h" // For `sample_map_point`

/* For the face type, NS = north-south, and EW = east-west.
If a face is NS, its two ends lie on a vertical top-down axis;
and if a face is EW, its two ends lie on a horizontal top-down axis.

Faces don't store their height origin, since sectors store that.
For vert faces, origin and size[0] are top-down, and size[1] is depth.
For hori faces, origin and size are both top-down. */

typedef struct {
	const enum {Flat, Vert_NS, Vert_EW} type;
	byte origin[2], size[2];
} Face;

//////////

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
static bool get_next_face(
	const byte varying_axis, const byte adjacent_side_val,
	const byte map_width, const byte* const heightmap,
	const Sector* const sector, Face* const face) {
	
	byte
		*const face_size = face -> size,
		*const face_origin_on_varying_axis = face -> origin + varying_axis;

	const byte
		max_visible_height = sector -> visible_heights.max,
		end_edge_val = sector -> origin[varying_axis] + sector -> size[varying_axis];

	byte face_height_diff = 0, start_val = *face_origin_on_varying_axis + face_size[0], map_point_params[2];
	map_point_params[!varying_axis] = adjacent_side_val;

	while (start_val < end_edge_val) {
		map_point_params[varying_axis] = start_val;

		const int16_t height_diff = max_visible_height - sample_map_point(
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

		const int16_t height_diff = max_visible_height - sample_map_point(
			heightmap, map_point_params[0], map_point_params[1], map_width);

		if (height_diff != face_height_diff) break;
		end_val++;
	}

	*face_origin_on_varying_axis = start_val;
	face_size[0] = end_val - start_val;
	face_size[1] = face_height_diff;

	return true;
}

static void add_to_face_mesh(List* const face_mesh,
	const Face face, const byte sector_max_visible_height,
	const byte side, const byte texture_id) {

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
			const byte size_z = face.size[0], size_y = face.size[1];
			const byte far_z = near_z + size_z, bottom_y = top_y - size_y;

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
			const byte size_x = face.size[0], size_y = face.size[1];
			const byte far_x = near_x + size_x, bottom_y = top_y - size_y;

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

void init_mesh_for_sector(
	const Sector* const sector, List* const face_mesh,
	byte* const biggest_face_height, const byte* const heightmap,
	const byte map_width, const byte map_height, const byte texture_id) {

	const byte
		dimensions[2] = {map_width, map_height},
		max_visible_height = sector -> visible_heights.max,
		*const origin_xz = sector -> origin,
		*const size_xz = sector -> size;

	add_to_face_mesh(face_mesh, // Adding the flat face
		(Face) {Flat, {origin_xz[0], origin_xz[1]}, {size_xz[0], size_xz[1]}},
		max_visible_height, 0, texture_id);

	//////////

	*biggest_face_height = 0;

	for (byte unvarying_axis = 0; unvarying_axis < 2; unvarying_axis++) {
		for (byte side = 0; side < 2; side++) {

			Face next_face = {
				.type = Vert_NS + unvarying_axis,
				.origin = {origin_xz[0], origin_xz[1]}
			};

			byte adjacent_side_val;

			if (side) { // `side` is a top or left of the top-down sector
				const byte unvarying_axis_origin = next_face.origin[unvarying_axis];
				if (unvarying_axis_origin == 0) continue;
				else adjacent_side_val = unvarying_axis_origin - 1;
			}
			else {
				byte* const unvarying_axis_origin = next_face.origin + unvarying_axis;
				if ((*unvarying_axis_origin += size_xz[unvarying_axis]) == dimensions[unvarying_axis]) continue;
				else adjacent_side_val = *unvarying_axis_origin;
			}

			while (get_next_face(!unvarying_axis, adjacent_side_val, map_width, heightmap, sector, &next_face)) {
				add_to_face_mesh(face_mesh, next_face, max_visible_height, side, texture_id);

				const byte face_height = next_face.size[1];
				if (face_height > *biggest_face_height) *biggest_face_height = face_height;
			}
		}
	}
}

List init_map_edge_mesh(const byte* const heightmap, const byte map_width, const byte map_height) {
	// TODO: make this a constant somewhere
	buffer_size_t submesh_amount_guess = (map_width + map_height) / 6;
	if (submesh_amount_guess == 0) submesh_amount_guess = 1;

	List edge_mesh = init_list(submesh_amount_guess, face_mesh_t);

	//////////

	for (byte unvarying_axis = 0; unvarying_axis < 2; unvarying_axis++) {
		Face face = {.type = Vert_NS + unvarying_axis, .size = {0, 0}};

		for (byte side = 0; side < 2; side++) { // `side` == top or left
			const byte map_size[2] = {map_width, map_height}, varying_axis = !unvarying_axis;

			face.origin[varying_axis] = 0;
			face.origin[unvarying_axis] = side ? 0 : (map_size[unvarying_axis] - 1);

			//////////

			bool building_edge_faces = true;

			while (building_edge_faces) {
				const byte face_height = sample_map_point(heightmap, face.origin[0], face.origin[1], map_width);

				byte map_pos[2] = {face.origin[0], face.origin[1]};
				byte* const varying_bottom_of_edge = map_pos + varying_axis;

				const byte varying_map_boundary = map_size[varying_axis];

				while (building_edge_faces && sample_map_point(heightmap,
					map_pos[0], map_pos[1], map_width) == face_height) {

					(*varying_bottom_of_edge)++;
					building_edge_faces = *varying_bottom_of_edge < varying_map_boundary;
				}

				face.size[0] = *varying_bottom_of_edge - face.origin[varying_axis];
				face.size[1] = face_height;

				//////////

				if (face_height != 0) { // No face mesh generated for height-zero faces
					byte* const unvarying_origin = face.origin + unvarying_axis;

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
