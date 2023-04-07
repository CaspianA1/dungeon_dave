#ifndef BUFFER_DEFS_H
#define BUFFER_DEFS_H

#include <stdint.h> // For sized ints

// TODO: make a `texture_size_t` type

//////////

typedef int8_t signed_byte;
typedef uint8_t byte;
typedef uint32_t buffer_size_t;

typedef struct {
	signed_byte x, y, z;
} sbvec3;

// Note: this texture id type does not apply to sectors' texture IDs.
typedef uint16_t material_index_t;
typedef uint16_t texture_id_t;
typedef uint16_t billboard_index_t;

//////////

typedef uint8_t map_texture_id_t;
typedef uint8_t map_pos_component_t; // This accounts for x, y, and z of the map (so also, all map heights)
typedef int16_t signed_map_pos_component_t; // Can contain a `map_pos_component_t`, with negative versions of it too

//////////

typedef struct {
	map_pos_component_t x, z;
} map_pos_xz_t;

typedef struct {
	map_pos_component_t* const data;
	const map_pos_xz_t size;
} Heightmap;

//////////

#define MATERIAL_INDEX_TYPENAME GL_UNSIGNED_SHORT
#define TEXTURE_ID_TYPENAME GL_UNSIGNED_SHORT
#define BILLBOARD_INDEX_TYPENAME GL_UNSIGNED_SHORT
#define MAP_POS_COMPONENT_TYPENAME GL_UNSIGNED_BYTE

#endif
