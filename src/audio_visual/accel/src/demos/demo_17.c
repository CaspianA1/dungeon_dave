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
	/* 3rd component specified by type.
	For vert faces, [0] indicates depth, and [1] indicates rel height from sector top. */
	byte origin[2], size[2];
} Face;

void print_face(const Face face, const char* const prefix_msg) {
	const char* const type_string =
		(char*[]) {"Flat", "Vert_NS", "Vert_EW"}[face.type];

	printf("%s{.type = %s, .origin = {%d, %d}, .size = {%d, %d}}\n",
		prefix_msg, type_string, face.origin[0],
		face.origin[1], face.size[0], face.size[1]);
}

// Assumes that the faces are vertical
void init_vert_ew_faces(const Sector sector, byte* const heightmap, const byte map_width) {
	if (sector.origin[1] == 0) return;

	const byte y_above = sector.origin[1] - 1;
	byte face_skippable = 0;

	int16_t last_height_diff = 0;
	Face curr_face = {Vert_EW, {sector.origin[0], sector.origin[1]}, {1, 0}};

	for (byte x = sector.origin[0]; x < sector.origin[0] + sector.size[0]; x++) {
		const byte height = *map_point(heightmap, x, y_above, map_width);
		const int16_t height_diff = sector.height - height;

		face_skippable = height_diff <= 0;

		if (!face_skippable) {
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
		// puts("---");
	}

	if (!face_skippable) print_face(curr_face, "Submit this contiguous face at the end: ");
}

#ifdef DEMO_17
int main(void) {
	enum {test_map_width = 8, test_map_height = 5};
	static const byte test_heightmap[test_map_height][test_map_width] = {
		{2, 2, 1, 1, 2, 5, 9, 7},
		{8, 8, 8, 8, 8, 8, 8, 8},
		{8, 8, 8, 8, 8, 8, 8, 8},
		{8, 8, 8, 8, 8, 8, 8, 8},
		{3, 3, 0, 0, 0, 0, 0, 0}
	};

	const SectorList sector_list = generate_sectors_from_heightmap((byte*) test_heightmap, test_map_width, test_map_height);
	// print_sector_list(&sector_list);

	for (int i = 0; i < sector_list.length; i++) {
		const Sector sector = sector_list.sectors[i];
		if (sector.height == 8) {
			init_vert_ew_faces(sector, (byte*) test_heightmap, test_map_width);
			break;
		}
	}

	free(sector_list.sectors);
}
#endif
