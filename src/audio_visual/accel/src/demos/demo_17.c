// This aims to optimize meshes when they are created, expanding upon sector_mesh.c.

#include "../utils.c"
#include "../sector_mesh.c"
#include "../sector.c"

typedef enum { // NS = north-sorth, and EW = east-west
	Flat, Vert_NS, Vert_EW
} FaceType;

typedef struct {
	const FaceType type;
	const byte origin[2]; // 3rd component specified by type
	byte size[2];
} Face;

// Assumes that the faces are vertical
void init_vert_ew_faces(const Sector sector, byte* const heightmap, const byte map_width) {
	// This is for top side right now

	if (sector.origin[0] == 0) return;

	// Store last adjacent height

	const byte y_above = sector.origin[1] - 1;

	for (byte x = sector.origin[0]; x < sector.origin[0] + sector.size[0]; x++) {
		const byte adjacent_height = *map_point(heightmap, x, y_above, map_width);
		(void) adjacent_height;
	}
}

#ifdef DEMO_17
int main(void) {
	DEBUG(sizeof(Face), zu);
	DEBUG(sizeof(FaceType), zu);
	DEBUG(sizeof(Face*), zu);

	enum {test_map_width = 8, test_map_height = 5};
	static const byte test_heightmap[test_map_height][test_map_width] = {
		{0, 2, 2, 0, 0, 0, 0, 0},
		{0, 2, 2, 1, 1, 1, 0, 0},
		{0, 2, 2, 1, 1, 1, 0, 0},
		{3, 3, 0, 0, 0, 0, 0, 0},
		{3, 3, 0, 0, 0, 0, 0, 0}
	};

	(void) test_heightmap;
}
#endif
