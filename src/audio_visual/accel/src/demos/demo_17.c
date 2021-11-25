// This aims to optimize meshes when they are created, expanding upon sector_mesh.c.

#include "../utils.c"
#include "../sector_mesh.c"
#include "../sector.c"

#include "../list.c"

typedef enum {
	/* NS - north-south, and EW = east-west.
	If a face is NS, its two ends lie on a vertical top-down axis;
	and if a fafe is EW, its two ends low on a horizontal axis. */
	Flat, Vert_NS, Vert_EW
} FaceType;

typedef struct {
	const FaceType type;
	/*  Faces don't store their height origin, since sectors store that.
	For vert faces, origin and size[0] are top-down, and size[1] is depth.
	For hori faces, origin and size are both top-down. */
	byte origin[2], size[2];
} Face;

void print_face(const Face face, const char* const prefix_msg) {
	const char* const type_string =
		(char*[]) {"Flat", "Vert_NS", "Vert_EW"}[face.type];

	printf("%s{.type = %s, .origin = {%d, %d}, .size = {%d, %d}}\n",
		prefix_msg, type_string, face.origin[0],
		face.origin[1], face.size[0], face.size[1]);
}

// Returns if there is anotehr face to get
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

void init_vert_faces(const Sector sector, const byte* const heightmap,
	const byte map_width, const byte map_height) {

	const byte dimensions[2] = {map_width, map_height};

	for (byte axis = 0; axis < 2; axis++) {
		for (byte side = 0; side < 2; side++) {
			Face next_face = {.type = Vert_NS + axis, .origin = {sector.origin[0], sector.origin[1]}};
			byte adjacent_side_val;

			if (side) { // Side is a top side or left side of the top-down sector
				if (next_face.origin[axis] == 0) continue;
				adjacent_side_val = next_face.origin[axis] - 1;
			}
			else {
				if ((next_face.origin[axis] += sector.size[axis]) == dimensions[axis]) continue;
				adjacent_side_val = next_face.origin[axis];
			}

			// TODO: store sectors in a generic container
			DEBUG(axis, d);

			while (get_next_face(sector, !axis, adjacent_side_val, map_width, heightmap, &next_face)) {
				print_face(next_face, "");
			}
			puts("---");
		}
	}
}

#ifdef DEMO_17
int main(void) {
	enum {test_map_width = 8, test_map_height = 5};
	static const byte test_heightmap[test_map_height][test_map_width] = {
		{2, 3, 3, 3, 4, 5, 5, 4},
		{3, 8, 8, 8, 8, 8, 8, 2},
		{4, 8, 8, 8, 8, 8, 8, 8},
		{5, 8, 8, 8, 8, 8, 8, 8},
		{0, 3, 3, 4, 0, 0, 9, 7}
	};

	const SectorList sector_list = generate_sectors_from_heightmap((byte*) test_heightmap, test_map_width, test_map_height);
	// print_sector_list(&sector_list);

	const List list = sector_list.list;
	for (size_t i = 0; i < list.length; i++) {
		const Sector sector = ((Sector*) list.data)[i];
		if (sector.height == 8) {
			init_vert_faces(sector, (byte*) test_heightmap, test_map_width, test_map_height);
			break;
		}
	}

	deinit_list(list); // vbo not freed through call to deinit_sector_list b/c vbo not initialized
}
#endif
