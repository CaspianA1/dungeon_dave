#ifndef SECTOR_H
#define SECTOR_H

#include "constants.h"
#include "buffer_defs.h"
#include "list.h"

const byte init_sector_alloc = 20;

typedef struct {
	const byte height, origin[2];
	byte size[2];
	// Start and end indices in ibo that encompass sectors' faces
	struct {index_type_t start, length;} ibo_range;
} Sector;

typedef struct {
	List sectors, indices;
	GLuint vbo, ibo;
	index_type_t* ibo_ptr;
} SectorList;

// Excluded: form_sector_area

void print_sector_list(const SectorList* const s);
void deinit_sector_list(const SectorList* const s);
byte* map_point(byte* const map, const byte x, const byte y, const byte map_width);

List generate_sectors_from_heightmap(const byte* const heightmap,
	const byte map_width, const byte map_height);

#endif