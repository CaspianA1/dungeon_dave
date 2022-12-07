#ifndef BITARRAY_H
#define BITARRAY_H

#include "utils/typedefs.h" // For `byte`
#include <limits.h> // For `CHAR_BIT`
#include "utils/alloc.h" // For `clearing_alloc`, and `dealloc`
#include <stdbool.h> // For `bool`

////////// The bitarray code is so small that it's all done as inline functions.

// A chunk is a unit of addressable data in the bitarray.
typedef uint8_t bitarray_chunk_t; // This must be unsigned

static const byte bytes_per_chunk = sizeof(bitarray_chunk_t);
static const byte bits_per_chunk = bytes_per_chunk * CHAR_BIT;

typedef struct {
	bitarray_chunk_t* chunks;
} BitArray;

////////// Private functions

static inline bitarray_chunk_t* get_bitarray_chunk(const BitArray bitarray, const buffer_size_t bit_index) {
	return bitarray.chunks + bit_index / bits_per_chunk;
}

static inline bitarray_chunk_t get_mask_for_bit_index_in_chunk(const buffer_size_t bit_index) {
	return (bitarray_chunk_t) ((bitarray_chunk_t) 1u << (bit_index % bits_per_chunk));
}

////////// Public functions

static inline BitArray init_bitarray(const buffer_size_t num_bits) {
	const buffer_size_t num_chunks = (num_bits - 1u) / bits_per_chunk + 1u;
	return (BitArray) {clearing_alloc(num_chunks, sizeof(bitarray_chunk_t))};
}

static inline void deinit_bitarray(const BitArray bitarray) {
	dealloc(bitarray.chunks);
}

static inline void set_bit_in_bitarray(const BitArray bitarray, const buffer_size_t bit_index) {
	*get_bitarray_chunk(bitarray, bit_index) |= get_mask_for_bit_index_in_chunk(bit_index);
}

// TODO: improve this via bit-level parallelism
static inline void set_bit_range_in_bitarray(const BitArray bitarray, const buffer_size_t start, const buffer_size_t end) {
	for (buffer_size_t i = start; i < end; i++) set_bit_in_bitarray(bitarray, i);
}

static inline bool bitarray_bit_is_set(const BitArray bitarray, const buffer_size_t bit_index) {
	return !!(*get_bitarray_chunk(bitarray, bit_index) & get_mask_for_bit_index_in_chunk(bit_index));
}

#endif
