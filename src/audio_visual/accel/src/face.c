#ifndef FACE_C
#define FACE_C

#include "headers/face.h"

void print_face(const Face face, const char* const prefix_msg) {
	const char* const type_string =
		(char*[]) {"Flat", "Vert_NS", "Vert_EW"}[face.type];

	printf("%s{.type = %s, .origin = {%d, %d}, .size = {%d, %d}}\n",
		prefix_msg, type_string, face.origin[0],
		face.origin[1], face.size[0], face.size[1]);
}

// Returns if there is another face to get
byte get_next_face(const Sector sector, const byte varying_axis,
	const byte adjacent_side_val, const byte map_width,
	const byte* const heightmap, Face* const face) {
	
	int16_t face_height_diff = 0;
	const byte end_edge_val = sector.origin[varying_axis] + sector.size[varying_axis];

	byte start_val = face -> origin[varying_axis] + face -> size[0], map_point_params[2];
	map_point_params[!varying_axis] = adjacent_side_val;

	while (start_val < end_edge_val) {
		map_point_params[varying_axis] = start_val;
		const int16_t height_diff = sector.height - *map_point((byte*) heightmap,
			map_point_params[0], map_point_params[1], map_width);

		if (height_diff > 0) {
			face_height_diff = height_diff;
			break;
		}
		start_val++;
	}

	if (start_val == end_edge_val) return 0;

	byte end_val = start_val;
	while (end_val < end_edge_val) {
		map_point_params[varying_axis] = end_val;
		const int16_t height_diff = sector.height - *map_point((byte*) heightmap,
			map_point_params[0], map_point_params[1], map_width);

		if (height_diff != face_height_diff) break;
		end_val++;
	}

	face -> origin[varying_axis] = start_val;
	face -> size[0] = end_val - start_val;
	face -> size[1] = face_height_diff;

	return 1;
}

void init_vert_faces(const Sector sector, List* const face_mesh_list, List* const index_list,
	const byte* const heightmap, const byte map_width, const byte map_height) {

	const byte dimensions[2] = {map_width, map_height};

	for (byte unvarying_axis = 0; unvarying_axis < 2; unvarying_axis++) {
		for (byte side = 0; side < 2; side++) {

			Face next_face = {
				.type = Vert_NS + unvarying_axis,
				.origin = {sector.origin[0], sector.origin[1]}
			};

			byte adjacent_side_val;

			if (side) { // Side is a top side or left side of the top-down sector
				if (next_face.origin[unvarying_axis] == 0) continue;
				adjacent_side_val = next_face.origin[unvarying_axis] - 1;
			}
			else {
				if ((next_face.origin[unvarying_axis] += sector.size[unvarying_axis])
					== dimensions[unvarying_axis]) continue;
				adjacent_side_val = next_face.origin[unvarying_axis];
			}

			void add_face_mesh_to_list(const Face, const byte, const byte, List* const, List* const);

			while (get_next_face(sector, !unvarying_axis, adjacent_side_val, map_width, heightmap, &next_face))
				add_face_mesh_to_list(next_face, sector.height, side, face_mesh_list, index_list);
		}
	}
}

void add_face_mesh_to_list(const Face face, const byte sector_height,
	const byte side, List* const face_mesh_list, List* const index_list) {

	const byte near_x = face.origin[0], near_z = face.origin[1];
	const mesh_type_t* face_mesh;

	switch (face.type) {
		case Flat: {
			const byte size_x = face.size[0], size_z = face.size[1];
			const byte far_x = near_x + size_x, far_z = near_z + size_z;

			face_mesh = (mesh_type_t[vars_per_face]) { // Top side - face 5, but UVs rotated
				near_x, sector_height, far_z, size_x, 0,
				far_x, sector_height, near_z, 0, size_z,
				near_x, sector_height, near_z, size_x, size_z,

				// near_x, sector_height, far_z, size_x, 0,
				far_x, sector_height, far_z, 0, 0
				// far_x, sector_height, near_z, 0, size_z
			};
			break;
		}
		case Vert_NS: {
			const byte size_z = face.size[0], size_y = face.size[1];
			const byte far_z = near_z + size_z, bottom_y = sector_height - size_y;

			face_mesh = side
				? (mesh_type_t[vars_per_face]) { // Left side - face 1
					near_x, bottom_y, near_z, 0, size_y,
					near_x, sector_height, far_z, size_z, 0,
					near_x, sector_height, near_z, 0, 0,

					// near_x, bottom_y, near_z, 0, size_y,
					near_x, bottom_y, far_z, size_z, size_y
					// near_x, sector_height, far_z, size_z, 0
				}
				: (mesh_type_t[vars_per_face]) { // Right side - face 2
					near_x, sector_height, near_z, size_z, 0,
					near_x, sector_height, far_z, 0, 0,
					near_x, bottom_y, near_z, size_z, size_y,

					// near_x, sector_height, far_z, 0, 0,
					near_x, bottom_y, far_z, 0, size_y
					// near_x, bottom_y, near_z, size_z, size_y
				};
			break;
		}
		case Vert_EW: {
			const byte size_x = face.size[0], size_y = face.size[1];
			const byte far_x = near_x + size_x, bottom_y = sector_height - size_y;

			face_mesh = side
				? (mesh_type_t[vars_per_face]) { // Bottom side - face 3
					near_x, sector_height, near_z, size_x, 0,
					far_x, sector_height, near_z, 0, 0,
					near_x, bottom_y, near_z, size_x, size_y,

					// far_x, sector_height, near_z, 0, 0,
					far_x, bottom_y, near_z, 0, size_y
					// near_x, bottom_y, near_z, size_x, size_y
				}
				: (mesh_type_t[vars_per_face]) { // Top side - face 4
					near_x, bottom_y, near_z, 0, size_y,
					far_x, sector_height, near_z, size_x, 0,
					near_x, sector_height, near_z, 0, 0,

					// near_x, bottom_y, near_z, 0, size_y,
					far_x, bottom_y, near_z, size_x, size_y
					// far_x, sector_height, near_z, size_x, 0
				};
			break;
		}
	}
	push_ptr_to_list(face_mesh_list, face_mesh);

	//////////

	const index_type_t s = index_list -> length * vertices_per_face; // s = index set start
	index_type_t index_set[indices_per_face] = {s, s + 1, s + 2, s, s + 3, s + 1};

	if ((face.type == Vert_NS && !side) || (face.type == Vert_EW && side)) {
		index_set[3]++;
		index_set[5]++;
	}

	push_ptr_to_list(index_list, index_set);
}

void init_face_mesh_and_sector_lists(SectorList* const sector_list,
	List* const face_mesh_list, const byte* const heightmap,
	const byte map_width, const byte map_height) {

	List sectors = generate_sectors_from_heightmap(heightmap, map_width, map_height);
	List index_list = init_list(sectors.length * 2.0f, index_type_t[indices_per_face]);
	*face_mesh_list = init_list(sectors.length * 1.8f, mesh_type_t[vars_per_face]);

	for (size_t i = 0; i < sectors.length; i++) {
		Sector* const sector_ref = ((Sector*) sectors.data) + i;
		sector_ref -> ibo_range.start = index_list.length * indices_per_face;

		const Sector sector = *sector_ref;
		const Face flat_face = {Flat, {sector.origin[0], sector.origin[1]}, {sector.size[0], sector.size[1]}};
		add_face_mesh_to_list(flat_face, sector.height, 0, face_mesh_list, &index_list);
		init_vert_faces(sector, face_mesh_list, &index_list, heightmap, map_width, map_height);

		sector_ref -> ibo_range.length =
			index_list.length * indices_per_face - sector_ref -> ibo_range.start;
	}

	*sector_list = (SectorList) {
		.sectors = sectors,
		.indices = index_list
	};
}

void init_sector_list_vbo_and_ibo(SectorList* const sector_list, const List* const face_list) {
	const size_t num_faces = face_list -> length;
	const GLsizeiptr
		total_vertex_bytes = num_faces * vars_per_face * sizeof(mesh_type_t),
		total_index_bytes = num_faces * sizeof(index_type_t[indices_per_face]);

	GLuint buffers[2];
	glGenBuffers(2, buffers);

	sector_list -> vbo = buffers[0];
	glBindBuffer(GL_ARRAY_BUFFER, sector_list -> vbo);
	glBufferData(GL_ARRAY_BUFFER, total_vertex_bytes, face_list -> data, GL_STATIC_DRAW);

	sector_list -> ibo = buffers[1];
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sector_list -> ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, total_index_bytes, sector_list -> indices.data, GL_DYNAMIC_DRAW);
}

void bind_sector_list_vbo_to_vao(const SectorList* const sector_list) {
	glBindBuffer(GL_ARRAY_BUFFER, sector_list -> vbo);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 3, MESH_TYPE_ENUM, GL_FALSE, bytes_per_vertex, NULL);
	glVertexAttribPointer(1, 2, MESH_TYPE_ENUM, GL_FALSE, bytes_per_vertex, (void*) (3 * sizeof(mesh_type_t)));
}

#endif
