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
#include "headers/texture.h"
#include "list.c"

//////////

/*
static void print_sector_list(const SectorList* const sector_list) {
	const List sectors = sector_list -> sectors;

	puts("sector_list = [");
	for (size_t i = 0; i < sectors.length; i++) {
		const Sector sector = ((Sector*) sectors.data)[i];
		printf("\t.texture_id = %d, {.origin = {%d, %d}, .size = {%d, %d}, "
			".visible_heights = {.min = %d, .max = %d}, "
			"ibo_range = {.start = %u, .range = %u}}%s\n",

			sector.texture_id, sector.origin[0], sector.origin[1], sector.size[0],
			sector.size[1], sector.visible_heights.min, sector.visible_heights.max,
			sector.ibo_range.start, sector.ibo_range.length, (i == sectors.length - 1) ? "" : ", "
		);
	}
	puts("]");
}
*/

void deinit_sector_list(const SectorList* const sector_list) {
	deinit_list(sector_list -> sectors);
	deinit_list(sector_list -> indices);

	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

	const GLuint buffers[2] = {sector_list -> vbo, sector_list -> ibo};
	glDeleteBuffers(2, buffers);
}

//////////

byte* map_point(byte* const map, const byte x, const byte y, const byte map_width) {
	return map + (y * map_width + x);
}

// Attributes here = height and texture id
byte point_matches_sector_attributes(const Sector* const sector_ref,
	const byte* const heightmap, const byte* const texture_id_map,
	const byte x, const byte y, const byte map_width) {

	return
		*map_point((byte*) heightmap, x, y, map_width) == sector_ref -> visible_heights.max
		&& *map_point((byte*) texture_id_map, x, y, map_width) == sector_ref -> texture_id;
}

// Gets length across, and then adds to area size y until out of map or length across not eq
Sector form_sector_area(Sector sector, const StateMap traversed_points,
	const byte* const heightmap, const byte* const texture_id_map,
	const byte map_width, const byte map_height) {

	byte top_right_corner_x = sector.origin[0];
	const byte origin_y = sector.origin[1];

	while (top_right_corner_x < map_width
		&& !get_statemap_bit(traversed_points, top_right_corner_x, origin_y)
		&& point_matches_sector_attributes(&sector, heightmap, texture_id_map, top_right_corner_x, origin_y, map_width)) {

		sector.size[0]++;
		top_right_corner_x++;
	}

	// Now, area.size[0] equals the first horizontal length of equivalent height found
	for (byte y = origin_y; y < map_height; y++, sector.size[1]++) {
		for (byte x = sector.origin[0]; x < top_right_corner_x; x++) {
			if (!point_matches_sector_attributes(&sector, heightmap, texture_id_map, x, y, map_width))
				goto clear_map_area;
		}
	}

	clear_map_area:

	for (byte y = origin_y; y < origin_y + sector.size[1]; y++) {
		for (byte x = sector.origin[0]; x < sector.origin[0] + sector.size[0]; x++)
			set_statemap_bit(traversed_points, x, y);
	}

	return sector;
}

List generate_sectors_from_maps(
	const byte* const heightmap, const byte* const texture_id_map,
	const byte map_width, const byte map_height) {

	// >> 3 = / 8. Works pretty well for my maps.
	const size_t sector_amount_guess = map_width * map_height >> 3;
	List sectors = init_list(sector_amount_guess, Sector);

	/* StateMap used instead of copy of heightmap with null map points, b/c 1. less bytes used
	and 2. for forming faces, will need original heightmap to be unmodified */
	const StateMap traversed_points = init_statemap(map_width, map_height);

	for (byte y = 0; y < map_height; y++) {
		for (byte x = 0; x < map_width; x++) {
			if (get_statemap_bit(traversed_points, x, y)) continue;

			const byte
				height = *map_point((byte*) heightmap, x, y, map_width),
				texture_id = *map_point((byte*) texture_id_map, x, y, map_width);

			if (texture_id >= MAX_NUM_TEXTURES) {
				fprintf(stderr, "Sector creation failure at pos {%d, %d}; texture ID = %d.\n", x, y, texture_id);
				fail("create a sector because the texture ID is too large", TextureIDIsTooLarge);
			}

			const Sector sector = form_sector_area((Sector) {
				.texture_id = texture_id,
				.origin = {x, y}, .size = {0, 0},
				.visible_heights = {.min = 0, .max = height}
			}, traversed_points, heightmap, texture_id_map, map_width, map_height);

			push_ptr_to_list(&sectors, &sector);
		}
	}

	deinit_statemap(traversed_points);
	return sectors;
}

#endif
