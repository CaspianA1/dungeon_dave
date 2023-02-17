#ifndef BITARRAY_H
#define BITARRAY_H

#include "utils/typedefs.h" // For `byte`
#include <limits.h> // For `CHAR_BIT`
#include <stdbool.h> // For `bool`
#include "utils/alloc.h" // For `clearing_alloc`, and `dealloc`

////////// The bitarray code is quite small, so it's all done as inline functions.

/*
- A chunk is a unit of addressable data in the bitarray.
- The type for the chunk must be unsigned.
- The number of bits per chunk must be a power of 2.
- The chunk size must be in the range of 8 to 64 bits (inclusive on both ends).
*/

typedef uint64_t bitarray_chunk_t;

static const bitarray_chunk_t chunk_with_all_bits_set = (bitarray_chunk_t) ~(bitarray_chunk_t) 0u;

static const byte
	log2_bits_per_chunk =
		(sizeof(bitarray_chunk_t) >= sizeof(uint8_t)) +
		(sizeof(bitarray_chunk_t) >= sizeof(uint16_t)) +
		(sizeof(bitarray_chunk_t) >= sizeof(uint32_t)) +
		(sizeof(bitarray_chunk_t) >= sizeof(uint64_t)) + 2u;

static const byte bits_per_chunk_minus_one =  sizeof(bitarray_chunk_t) * CHAR_BIT - 1u;

//////////

typedef struct {
	bitarray_chunk_t* chunks;
} BitArray;

////////// Private utils

static inline bitarray_chunk_t* get_bitarray_chunk(const BitArray bitarray, const buffer_size_t bit_index) {
	// Getting the chunk index via right-shifting by the log2 of the divisor, instead of dividing
	return bitarray.chunks + (bit_index >> log2_bits_per_chunk);
}

static inline bitarray_chunk_t get_mask_for_bit_index_in_chunk(const buffer_size_t bit_index) {
	// Left-shifting one bit by the modulus of the number of bits per chunk, with `&` instead of `%`
	return (bitarray_chunk_t) ((bitarray_chunk_t) 1u << (bit_index & bits_per_chunk_minus_one));
}

////////// Public utils

static inline bool bitarray_bit_is_set(const BitArray bitarray, const buffer_size_t bit_index) {
	// `&`-ing the given chunk by the mask for the given bit index, and getting 0 or 1 from that
	return !!(*get_bitarray_chunk(bitarray, bit_index) & get_mask_for_bit_index_in_chunk(bit_index));
}

static inline void set_bit_in_bitarray(const BitArray bitarray, const buffer_size_t bit_index) {
	// `|`-ing the given chunk by the mask for the given bit index
	*get_bitarray_chunk(bitarray, bit_index) |= get_mask_for_bit_index_in_chunk(bit_index);
}

// This is inclusive on both ends.
void set_bit_range_in_bitarray(const BitArray bitarray, const buffer_size_t start, const buffer_size_t end);

////////// Init and deinit

static inline BitArray init_bitarray(const buffer_size_t num_bits) {
	/* Uses the equation for integer ceiling division, but right-
	shifting by the log2 of the divisor, instead of dividing */
	const buffer_size_t num_chunks = ((num_bits - 1u) >> log2_bits_per_chunk) + 1u;
	return (BitArray) {clearing_alloc(num_chunks, sizeof(bitarray_chunk_t))};
}

static inline void deinit_bitarray(const BitArray bitarray) {
	dealloc(bitarray.chunks);
}

#endif
