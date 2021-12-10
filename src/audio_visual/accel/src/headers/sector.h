#ifndef SECTOR_H
#define SECTOR_H

#include "constants.h"
#include "buffer_defs.h"
#include "drawable_set.h"
#include "list.h"

typedef struct {
	const byte texture_id, origin[2];
	byte size[2]; // Top-down (X and Z); same for origin
	struct {byte min; const byte max;} visible_heights;
	struct {buffer_index_t start, length;} ibo_range; // ibo domain that defines sector's faces
} Sector;

// Excluded: print_sector_list, form_sector_area

byte* map_point(byte* const map, const byte x, const byte y, const byte map_width);

List generate_sectors_from_maps(
	const byte* const heightmap, const byte* const texture_id_map,
	const byte map_width, const byte map_height);

#endif
