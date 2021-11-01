#include "demo_11.c"
/*
- Sectors contain their meshes
- To begin with, don't clip sector heights based on adjacent heights
- Sectors are rectangular

- Not perfect, but sectors + their meshes for clipping and rendering, and texmaps + heightmaps for game logic
- Ideal: BSPs, but not worth time
- To start, one vbo + texture per sector
- Form sectors of height 0 too, but do that later
*/

typedef struct {
	const byte height, origin[2];
	byte size[2];
} SectorArea;

typedef struct {
	const SectorArea area;
	const GLuint vbo, texture;
} Sector;

typedef struct {
	Sector* const sectors;
	int length, max_alloc;
} SectorStack;

byte* map_point(byte* const map, const byte x, const byte y, const byte map_width) {
	return map + (y * map_width + x);
}

byte area_is_valid(const SectorArea* const area, byte* const map, const byte map_width) {
	const byte start_x = area -> origin[0], start_y = area -> origin[1];
	const byte end_x = start_x + area -> size[0], end_y = area -> size[1];

	for (byte y = start_y; y < end_y; y++) {
		for (byte x = start_x; x < end_x; x++) {
			if (*map_point(map, x, y, map_width) != area -> height) return 0;
		}
	}
	return 1;
}

// Corner is top left
SectorArea attempt_area_fill(SectorArea area, byte* const map, const byte map_width, const byte map_height) {
	// Not working correctly yet
	for (byte y = area.origin[1]; y < map_height; y++) {
		for (byte x = area.origin[0]; x < map_width; x++) {
			if (area_is_valid(&area, map, map_width)) area.size[0]++;
			else break;
		}
		if (area_is_valid(&area, map, map_width)) area.size[1]++;
		else break;
	}

	for (byte y = area.origin[1]; y < area.origin[1] + area.size[1]; y++) {
		for (byte x = area.origin[0]; x < area.origin[0] + area.size[0]; x++)
			*map_point(map, x, y, map_width) = 0;
	}

	return area;
}

SectorStack* generate_sectors_from_heightmap(byte* const heightmap, const byte map_width, const byte map_height) {
	SectorStack* const sectors = NULL;

	for (byte y = 0; y < map_height; y++) {
		for (byte x = 0; x < map_width; x++) {
			const byte height = *map_point(heightmap, x, y, map_width);
			if (height == 0) continue;
			const SectorArea area = {.height = height, .origin = {x, y}, .size = {1, 1}};
			const SectorArea expanded = attempt_area_fill(area, heightmap, map_width, map_height);
			printf("expanded = {.height = %d, .origin = {%d, %d}, .size = {%d, %d}}\n",
				expanded.height, expanded.origin[0], expanded.origin[1], expanded.size[0], expanded.size[1]);
			// Clear area, and add to sectors
		}
	}

	return sectors;
}

StateGL demo_12_init(void) {
	StateGL sgl = demo_11_init();

	enum {map_width = 8, map_height = 10};
	const byte heightmap[map_height][map_width] = {
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 2, 2, 0, 0},
		{0, 0, 0, 1, 2, 2, 0, 0},
		{0, 0, 1, 1, 1, 1, 0, 0},
		{0, 0, 1, 1, 1, 1, 0, 0},
		{0, 0, 1, 1, 1, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0}
	};

	generate_sectors_from_heightmap((byte*) heightmap, map_width, map_height);

	return sgl;
}

#ifdef DEMO_12
int main(void) {
	make_application(demo_11_drawer, demo_12_init, deinit_demo_vars);
}
#endif
