// This aims to optimize meshes when they are created, expanding upon sector_mesh.c.

#include "../utils.c"
#include "../sector_mesh.c"
#include "../sector.c"

typedef enum { // NS = north-sorth, and EW = east-west
	Flat, Vert_NS, Vert_EW
} FaceType;

// Faces don't store their height beginning, since sectors will store that
typedef struct {
	const FaceType type;
	/* For vert faces, origin is top-down, size[0] is top-down length, and size[1] is depth.
	For hori faces, origin and size are top-down. */
	byte origin[2], size[2];
} Face;

void print_face(const Face face, const char* const prefix_msg) {
	const char* const type_string =
		(char*[]) {"Flat", "Vert_NS", "Vert_EW"}[face.type];

	printf("%s{.type = %s, .origin = {%d, %d}, .size = {%d, %d}}\n",
		prefix_msg, type_string, face.origin[0],
		face.origin[1], face.size[0], face.size[1]);
}

// Returns if there's another face to get
byte get_next_ew_face(const Sector sector, const byte adjacent_y,
	const byte map_width, const byte* const heightmap, Face* const face) {
	
	int16_t face_height_diff = 0;
	const byte right_edge_x = sector.origin[0] + sector.size[0];
	byte start_x = face -> origin[0] + face -> size[0];

	while (start_x < right_edge_x) { // Find starting point for face, where face will be visible against adjacent sector
		const int16_t height_diff = sector.height - *map_point((byte*) heightmap, start_x, adjacent_y, map_width);
		if (height_diff > 0) { // If face will be visible against adjacent side
			face_height_diff = height_diff;
			break;
		}
		start_x++;
	}

	// If no face start was found, done with finding faces
	if (start_x == right_edge_x) return 0;

	byte end_x = start_x;
	while (end_x < right_edge_x) { // Extend the face's side until a height discontinuity is found
		const int16_t height_diff = sector.height - *map_point((byte*) heightmap, end_x, adjacent_y, map_width);
		if (height_diff != face_height_diff) break;
		end_x++;
	}

	face -> origin[0] = start_x;
	face -> size[0] = end_x - start_x;
	face -> size[1] = face_height_diff;

	printf("%d to %d\n", start_x, end_x - 1);
	return 1;
}

// Sides of top-down sector
byte get_next_ns_face(const Sector sector, const byte adjacent_x,
	const byte map_width, const byte* const heightmap, Face* const face) {

	int16_t face_height_diff = 0;
	const byte bottom_edge_y = sector.origin[1] + sector.size[1];
	byte start_y = face -> origin[1] + face -> size[1];

	(void) face_height_diff;

	while (start_y < bottom_edge_y) {
		const int16_t height_diff = sector.height - *map_point((byte*) heightmap, adjacent_x, start_y, map_width);
		(void) height_diff;
	}
	return 1;
}

/*
Generic:
- Face axis
- `is_top` to `is_top_or_left`
- Determine type from face axis
- `adjacent_y` to adjacent_axis_val
- Map width and height into an array, to index it for out of bounds checking
- Find opp_axis
- `init_vert_faces_for_axis`
- `start_x` to `start_axis_val`, and same for `end_x`
- Check map point val in some way depending on axis val and adjacency
*/

// Param 'is_top' indicates if EW face is on top or bottom side of 2D sector
void init_vert_ew_faces(const Sector sector, const byte* const heightmap,
	const byte map_width, const byte map_height, const byte is_top) {

	Face next_face = {.type = Vert_EW, .origin = {sector.origin[0], sector.origin[1]}};
	byte adjacent_y;

	if (is_top) {
		if (next_face.origin[1] == 0) return;
		adjacent_y = next_face.origin[1] - 1;
	}
	else {
		if ((next_face.origin[1] += sector.size[1]) == map_height) return;
		adjacent_y = next_face.origin[1];
	}

	while (get_next_ew_face(sector, adjacent_y, map_width, heightmap, &next_face)) {
		print_face(next_face, "");
	}
}

#ifdef DEMO_17
int main(void) {
	enum {test_map_width = 8, test_map_height = 5};
	static const byte test_heightmap[test_map_height][test_map_width] = {
		{2, 2, 4, 9, 9, 1, 7, 7},
		{0, 8, 8, 8, 8, 8, 8, 8},
		{0, 8, 8, 8, 8, 8, 8, 8},
		{0, 8, 8, 8, 8, 8, 8, 8},
		{0, 3, 3, 4, 0, 0, 9, 9}

		/*
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 8, 8, 8, 8, 8, 8, 0},
		*/
	};

	const SectorList sector_list = generate_sectors_from_heightmap((byte*) test_heightmap, test_map_width, test_map_height);
	// print_sector_list(&sector_list);

	for (int i = 0; i < sector_list.length; i++) {
		const Sector sector = sector_list.sectors[i];
		if (sector.height == 8) {
			init_vert_ew_faces(sector, (byte*) test_heightmap, test_map_width, test_map_height, 1);
			puts("---");
			init_vert_ew_faces(sector, (byte*) test_heightmap, test_map_width, test_map_height, 0);
			break;
		}
	}

	free(sector_list.sectors);
}
#endif
