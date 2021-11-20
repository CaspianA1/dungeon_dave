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
	/* 3rd component specified by type. For vert faces, [0] indicates
	axis dist, and [1] indicates rel downwards-dist from sector top. */
	byte origin[2], size[2];
} Face;

void print_face(const Face face, const char* const prefix_msg) {
	const char* const type_string =
		(char*[]) {"Flat", "Vert_NS", "Vert_EW"}[face.type];

	printf("%s{.type = %s, .origin = {%d, %d}, .size = {%d, %d}}\n",
		prefix_msg, type_string, face.origin[0],
		face.origin[1], face.size[0], face.size[1]);
}

/*
void generic_init_vert_ew_faces(const Sector sector, byte* const heightmap, const byte map_width,
	const byte map_height, const byte is_bottom_side) {

	const byte start_x = sector.origin[0];
	byte start_y = sector.origin[1];

	if (is_bottom_side) {
		start_y += sector.size[1] - 1;

		DEBUG(sector.origin[1], d);
		DEBUG(sector.size[1], d);
		DEBUG(start_y, d);
		if (start_y == map_height - 1) return;
	}
	else if (start_y == 0) return;

	//////////

	const byte adjacent_y = start_y + (is_bottom_side ? 1 : -1);
	byte face_skippable = 0;

	int16_t last_height_diff = 0;
	Face curr_face = {Vert_EW, {start_x, start_y}, {1, 0}};

	for (byte x = start_x; x < start_x + sector.size[0]; x++) {
		const byte height = *map_point(heightmap, x, adjacent_y, map_width);
		const int16_t height_diff = sector.height - height;

		const byte not_skippable_before = !face_skippable;
		face_skippable = height_diff <= 0;

		if (not_skippable_before && face_skippable) {
			puts("Continuity error");
		}

		if (!face_skippable) {
			if (height_diff == last_height_diff) curr_face.size[0]++;
			else {
				if (last_height_diff != 0) {
					print_face(curr_face, "Face: ");
					curr_face.origin[0] = x;
					curr_face.size[0] = 1;
				}
				curr_face.size[1] = height_diff;
			}
		}
		last_height_diff = height_diff;
	}
	if (!face_skippable) print_face(curr_face, "End face: ");
}
*/

// Assumes that the faces are vertical
/*
void init_vert_ew_faces(const Sector sector, byte* const heightmap, const byte map_width) {
	// == edge
	if (sector.origin[1] == 0) return;

	// Side decr
	const byte y_above = sector.origin[1] - 1;

	byte face_skippable = 1;

	int16_t last_height_diff = 0;
	Face curr_face = {Vert_EW, {sector.origin[0], sector.origin[1]}, {1, 0}};

	for (byte x = sector.origin[0]; x < sector.origin[0] + sector.size[0]; x++) {
		const byte height = *map_point(heightmap, x, y_above, map_width);
		const int16_t height_diff = sector.height - height;

		const byte had_face_before = !face_skippable;
		face_skippable = height_diff <= 0;

		if (face_skippable && had_face_before) {
			puts("Catch it");
			print_face(curr_face, "Face: ");
			curr_face.origin[0] = x;
			curr_face.size[0] = 1; // top-down x
		}

		else if (!face_skippable) {
			if (height_diff == last_height_diff) curr_face.size[0]++;
			else {
				if (last_height_diff != 0) { // Add face to list here
					print_face(curr_face, "Face: ");
					curr_face.origin[0] = x;
					curr_face.size[0] = 1; // top-down x
				}
				curr_face.size[1] = height_diff; // top-down y
			}
		}
		last_height_diff = height_diff;
	}

	if (!face_skippable) print_face(curr_face, "End face: ");
}
*/

// Returns if there's another face to get
byte get_next_ew_face(const byte sector_height, const byte adjacent_y, const byte map_width,
	byte* const x_ref, byte* const heightmap, Face* const face) {

	(void) face;

	// While bad height, progress; otherwise, add on until bad height

	byte start_x = *x_ref;
	int16_t span_height_diff = 0;

	while (start_x < map_width) {
		const int16_t height_diff = sector_height - *map_point(heightmap, start_x++, adjacent_y, map_width);
		if (height_diff != 0) {
			span_height_diff = height_diff;
			break;
		}
	}

	byte end_x = start_x;
	// Here, extend with height diff

	*x_ref = end_x;

	return !(end_x == map_width || span_height_diff <= 0);
}

// Uses while loops instead
void init_vert_ew_faces(const Sector sector, byte* const heightmap, const byte map_width) {
	if (sector.origin[1] == 0) return;

	const byte y_above = sector.origin[1] - 1;
	// Face curr_face = {Vert_EW, {sector.origin[0], sector.origin[1]}, {1, 0}};

	byte x = 0;
	Face next_face;

	while (get_next_ew_face(sector.height, y_above, map_width, &x, heightmap, &next_face)) {

	}
}

#ifdef DEMO_17
int main(void) {
	enum {test_map_width = 8, test_map_height = 5};
	static const byte test_heightmap[test_map_height][test_map_width] = {
		{2, 2, 1, 1, 2, 9, 9, 2}, // This breaks stuff
		// {2, 2, 1, 1, 1, 9, 8, 9},
		// {2, 2, 2, 2, 2, 9, 9, 9},
		// {2, 9, 2, 9, 9, 9, 9, 9},
		{8, 8, 8, 8, 8, 8, 8, 8},
		{8, 8, 8, 8, 8, 8, 8, 8},
		{8, 8, 8, 8, 8, 8, 8, 8},
		{0, 3, 3, 0, 0, 0, 9, 9}
	};

	const SectorList sector_list = generate_sectors_from_heightmap((byte*) test_heightmap, test_map_width, test_map_height);
	// print_sector_list(&sector_list);

	for (int i = 0; i < sector_list.length; i++) {
		const Sector sector = sector_list.sectors[i];
		if (sector.height == 8) {
			init_vert_ew_faces(sector, (byte*) test_heightmap, test_map_width);
			// generic_init_vert_ew_faces(sector, (byte*) test_heightmap, test_map_width, test_map_height, 0);
			// break;
		}
	}

	free(sector_list.sectors);
}
#endif
