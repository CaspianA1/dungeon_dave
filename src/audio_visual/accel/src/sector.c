typedef struct {
	const byte height, origin[2];
	byte size[2];
} SectorArea;

typedef struct {
	const SectorArea area;
	GLuint vbo; // assoc texture later
} Sector;

typedef struct {
	Sector* data;
	int length, max_alloc;
} SectorList;

// This assumes that no map points will have a value of 255
const byte NULL_MAP_POINT = 255;
const float sector_realloc_rate = 1.5;

SectorList init_sector_list(const int init_size) {
	return (SectorList) {.data = malloc(init_size * sizeof(Sector)), 0, init_size};
}

void push_to_sector_list(SectorList* const s, const Sector* const sector) {
	if (s -> length == s -> max_alloc)
		s -> data = realloc(s -> data, (s -> max_alloc *= sector_realloc_rate) * sizeof(Sector));

	memcpy(s -> data + s -> length++, sector, sizeof(Sector));
}

void print_sector_list(SectorList* const s) {
	puts("[");
	for (int i = 0; i < s -> length; i++) {
		const SectorArea* const area = &s -> data[i].area;
		printf("\t{.height = %d, .origin = {%d, %d}, .size = {%d, %d}}\n",
				area -> height, area -> origin[0], area -> origin[1], area -> size[0], area -> size[1]);
	}
	puts("]");
}

#define deinit_sector_list(s) free(s.data)

//////////

byte* map_point(byte* const map, const byte x, const byte y, const byte map_width) {
	return map + (y * map_width + x);
}

// Gets length acros, and then adds to area size y until out of map or length across not eq
SectorArea get_sector_area(SectorArea area, byte* const map, const byte map_width, const byte map_height) {
	byte top_right_corner = area.origin[0];

	while (top_right_corner < map_width && *map_point(map, top_right_corner, area.origin[1], map_width) == area.height) {
		area.size[0]++;
		top_right_corner++;
	}

	// Now, area.size[0] equals the first horizontal length of equivalent height found
	for (byte y = area.origin[1]; y < map_height; y++, area.size[1]++) {
		for (byte x = area.origin[0]; x < top_right_corner; x++) {
			// If consecutive heights didn't continue
			if (*map_point(map, x, y, map_width) != area.height)
				goto clear_map_area;
		}
	}

	clear_map_area:

	for (byte y = area.origin[1]; y < area.origin[1] + area.size[1]; y++)
		memset(map_point(map, area.origin[0], y, map_width), NULL_MAP_POINT, area.size[0]);

	return area;
}

SectorList generate_sectors_from_heightmap(const byte* const heightmap, const byte map_width, const byte map_height) {
	SectorList sectors = init_sector_list(5);

	/* A copy of the heightmap is made b/c the heightmap data must
	be modified, and the heightmap param should stay constant */
	const size_t heightmap_bytes = map_width * map_height;
	byte* const heightmap_copy = malloc(heightmap_bytes);
	memcpy(heightmap_copy, heightmap, heightmap_bytes);

	for (byte y = 0; y < map_height; y++) {
		for (byte x = 0; x < map_width; x++) {
			const byte height = *map_point(heightmap_copy, x, y, map_width);
			if (height == NULL_MAP_POINT) continue;
			const SectorArea area_seed = {.height = height, .origin = {x, y}, .size = {0, 0}};
			const SectorArea expanded_area = get_sector_area(area_seed, heightmap_copy, map_width, map_height);
			const Sector sector = {expanded_area, 0};
			push_to_sector_list(&sectors, &sector);
		}
	}

	free(heightmap_copy);
	return sectors;
}
