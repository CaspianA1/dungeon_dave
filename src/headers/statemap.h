#ifndef STATEMAP_H
#define STATEMAP_H

#include "utils.h"

// This should not be signed
typedef byte statemap_chunk_t;

typedef struct {
	const buffer_size_t chunks_across;
	statemap_chunk_t* data;
} StateMap;

// Excluded: get_statemap_chunk, get_mask_for_bit_index_in_chunk

#define deinit_statemap(s) free(s.data)

StateMap init_statemap(const buffer_size_t bits_across, const buffer_size_t bits_down);
void set_statemap_bit(const StateMap statemap, const buffer_size_t bits_x, const buffer_size_t bits_y);
void set_statemap_area(const StateMap statemap, const buffer_size_t area[4]);
bool statemap_bit_is_set(const StateMap statemap, const buffer_size_t bits_x, const buffer_size_t bits_y);

#endif
