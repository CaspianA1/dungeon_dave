#ifndef SECTOR_C
#define SECTOR_C

#define inlinable static inline
#define wmalloc malloc
#define wfree free

#define bit_is_set(bits, mask) ((bits) & (mask))
#define set_bit(bits, mask) ((bits) |= (mask))

typedef struct {
	int chunk_dimensions[2];
	size_t alloc_bytes;
	byte* data;
} StateMap;

#include "../../../main/statemap.c"
#include "headers/sector.h"
#include "list.c"

//////////

void print_sector_list(const SectorList* const s) {
	const List sectors = s -> sectors;

	puts("[");
	for (size_t i = 0; i < sectors.length; i++) {
		const Sector* const sector = ((Sector*) sectors.data) + i;
		// const Sector* const sector = &s -> sectors[i];
		printf("\t{.height = %d, .origin = {%d, %d}, .size = {%d, %d}}\n",
			sector -> height, sector -> origin[0], sector -> origin[1],
			sector -> size[0], sector -> size[1]);
	}
	puts("]");
}

void deinit_sector_list(const SectorList* const s) {
	deinit_list(s -> sectors);
	deinit_list(s -> indices);

	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

	const GLuint buffers[2] = {s -> vbo, s -> ibo};
	glDeleteBuffers(2, buffers);
}

//////////

byte* map_point(byte* const map, const byte x, const byte y, const byte map_width) {
	return map + (y * map_width + x);
}

// Gets length across, and then adds to area size y until out of map or length across not eq
Sector form_sector_area(Sector sector, const StateMap traversed_points,
	const byte* const map, const byte map_width, const byte map_height) {

	byte top_right_corner = sector.origin[0];

	while (top_right_corner < map_width
		&& *map_point((byte*) map, top_right_corner, sector.origin[1], map_width) == sector.height
		&& !get_statemap_bit(traversed_points, top_right_corner, sector.origin[1])) {

		sector.size[0]++;
		top_right_corner++;
	}

	// Now, area.size[0] equals the first horizontal length of equivalent height found
	for (byte y = sector.origin[1]; y < map_height; y++, sector.size[1]++) {
		for (byte x = sector.origin[0]; x < top_right_corner; x++) {
			// If consecutive heights didn't continue
			if (*map_point((byte*) map, x, y, map_width) != sector.height)
				goto clear_map_area;
		}
	}

	clear_map_area:

	for (byte y = sector.origin[1]; y < sector.origin[1] + sector.size[1]; y++) {
		for (byte x = sector.origin[0]; x < sector.origin[0] + sector.size[0]; x++)
			set_statemap_bit(traversed_points, x, y);
	}

	return sector;
}

List generate_sectors_from_heightmap(const byte* const heightmap,
	const byte map_width, const byte map_height) {

	List sectors = init_list(init_sector_alloc, Sector);

	/* StateMap used instead of copy of heightmap with null map points, b/c 1. less bytes used
	and 2. for forming faces, will need original heightmap to be unmodified */
	const StateMap traversed_points = init_statemap(map_width, map_height);

	for (byte y = 0; y < map_height; y++) {
		for (byte x = 0; x < map_width; x++) {
			if (get_statemap_bit(traversed_points, x, y)) continue;

			const byte height = *map_point((byte*) heightmap, x, y, map_width);

			const Sector seed_sector = {.height = height, .origin = {x, y}, .size = {0, 0}};
			const Sector expanded_sector = form_sector_area(
				seed_sector, traversed_points, heightmap, map_width, map_height);

			push_ptr_to_list(&sectors, &expanded_sector);
		}
	}

	deinit_statemap(traversed_points);
	return sectors;
}

#endif
