#include "demo_11.c"
/*
- Sectors contain their meshes
- To begin with, don't clip sector heights based on adjacent heights
- Sectors are rectangular

- It's a bit much storing maps as texmaps, heightmaps, and sectors with associated edges
- What shall I do

- If I do only sectors, point height lookup would be slower for game logic
- And only texmaps + heightmaps means no 3D

- So perhaps both: sectors for clipping + sector meshes for rendering, and texmaps + heightmaps for game logic
- That should be okay, given the speed of modern computers - that way, I keep the game logic + keep it fast,
- And allow for 3d rendering
*/

typedef struct {
	byte height, origin[2], size[2];
} Sector;

void generate_sectors_from_heightmap(byte* const heightmap, const byte map_width, const byte map_height) {
	(void) heightmap;
	for (byte y = 0; y < map_height; y++) {
		for (byte x = 0; x < map_width; x++) {
		}
	}
}

StateGL demo_12_init(void) {
	StateGL sgl = demo_10_init();

	enum {map_width = 8, map_height = 5};
	const byte heightmap[map_height][map_width] = {
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 1, 2, 0, 0, 0},
		{0, 0, 0, 1, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0}
	};

	generate_sectors_from_heightmap((byte*) heightmap, map_width, map_height);

	return sgl;
}

#ifdef DEMO_12
int main(void) {
	make_application(demo_10_drawer, demo_12_init, deinit_demo_vars);
}
#endif
