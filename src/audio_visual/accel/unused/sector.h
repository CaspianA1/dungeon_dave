#ifndef SECTOR_H
#define SECTOR_H

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

#include "list.c" // TODO: remove this and the line below
#include "../../../main/statemap.c"

//////////

const byte init_sector_alloc = 20;

typedef struct {
	const byte height, origin[2];
	byte size[2]; // TODO: below member to index_type_t
	struct {int32_t start, end;} ibo_range; // start and end indices in ibo that encompass sectors' faces
} Sector;

typedef struct {
	List list;
	GLuint vbo, ibo;
	GLsizei num_vertices;
} SectorList;

SectorList init_sector_list(void);
void print_sector_list(const SectorList* const sector_list);
void deinit_sector_list(const SectorList* const sector_list);
byte* map_point(byte* const map, const byte x, const byte y, const byte map_width);



#endif
