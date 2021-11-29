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
	struct {index_type_t start, end;} ibo_range;
} Sector;

typedef struct {
	List list;
	GLuint vbo, ibo;
} SectorList;

// Excluded: form_sector_area

SectorList init_sector_list(void);
void print_sector_list(const SectorList* const s);
void deinit_sector_list(const SectorList* const s);
byte* map_point(byte* const map, const byte x, const byte y, const byte map_width);

SectorList generate_sectors_from_heightmap(const byte* const heightmap,
	const byte map_width, const byte map_height);



#endif
