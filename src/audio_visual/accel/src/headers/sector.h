#ifndef SECTOR_H
#define SECTOR_H

#include "constants.h"
#include "buffer_defs.h"
#include "list.h"

typedef struct {
	const byte texture_id, origin[2];
	byte size[2]; // Top-down (X and Z); same for origin
	struct {byte min; const byte max;} visible_heights;
	struct {index_type_t start, length;} ibo_range; // ibo domain that defines sector's faces
} Sector;

typedef struct {
	List sectors, indices;
	GLuint vbo, ibo;
	index_type_t* ibo_ptr;
} SectorList;

// Excluded: print_sector_list, form_sector_area

void deinit_sector_list(const SectorList* const sector_list);
byte* map_point(byte* const map, const byte x, const byte y, const byte map_width);

List generate_sectors_from_maps(
	const byte* const heightmap, const byte* const texture_id_map,
	const byte map_width, const byte map_height);

#endif
