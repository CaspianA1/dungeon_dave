#ifndef BITARRAY_H
#define BITARRAY_H

#include "utils/typedefs.h" // For `byte`
#include <limits.h> // For `CHAR_BIT`
#include <stdbool.h> // For `bool`

// TODO: use this for the statemap, and for the dict, to see if entries are filled

// A chunk is a unit of addressable data in the bitarray.
typedef uint8_t bitarray_chunk_t; // This must be unsigned

static const byte bytes_per_chunk = sizeof(bitarray_chunk_t);
static const byte bits_per_chunk = bytes_per_chunk * CHAR_BIT;

typedef struct {
	bitarray_chunk_t* const chunks;
} BitArray;

// Excluded: get_bitarray_chunk, get_mask_for_bit_index_in_chunk

BitArray init_bitarray(const buffer_size_t num_bits);
void deinit_bitarray(const BitArray bitarray);

void set_bit_in_bitarray(const BitArray bitarray, const buffer_size_t bit_index);
bool bitarray_bit_is_set(const BitArray bitarray, const buffer_size_t bit_index);

#endif
