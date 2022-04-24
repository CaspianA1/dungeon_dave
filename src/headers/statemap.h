#ifndef STATEMAP_H
#define STATEMAP_H

#include "utils.h"

typedef struct {
	unsigned chunk_dimensions[2];
	buffer_size_t alloc_bytes;
	byte* data;
} StateMap;

// Excluded: statemap_byte, get_n_for_bits_x

#define clear_statemap(s) memset(s.data, 0, s.alloc_bytes)
#define deinit_statemap(s) free(s.data)

StateMap init_statemap(const unsigned bits_across, const unsigned bits_down);
void set_statemap_bit(const StateMap statemap, const unsigned bits_x, const unsigned bits_y);
byte get_statemap_bit(const StateMap statemap, const unsigned bits_x, const unsigned bits_y);

#endif
