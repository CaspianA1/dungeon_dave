#include "utils/bitarray.h"
#include "utils/alloc.h" // For `clearing_alloc`, and `dealloc`
#include "utils/macro_utils.h" // For `CHECK_BITMASK`

////////// Init and deinit

BitArray init_bitarray(const buffer_size_t num_bits) {
	const buffer_size_t num_chunks = (num_bits - 1u) / bits_per_chunk + 1u;
	return (BitArray) {clearing_alloc(num_chunks, sizeof(bitarray_chunk_t))};
}

void deinit_bitarray(const BitArray bitarray) {
	dealloc(bitarray.chunks);
}

////////// Setting and getting

static bitarray_chunk_t* get_bitarray_chunk(const BitArray bitarray, const buffer_size_t bit_index) {
	return bitarray.chunks + bit_index / bits_per_chunk;
}

static bitarray_chunk_t get_mask_for_bit_index_in_chunk(const buffer_size_t bit_index) {
	return (bitarray_chunk_t) ((bitarray_chunk_t) 1u << (bit_index % bits_per_chunk));
}

void set_bit_in_bitarray(const BitArray bitarray, const buffer_size_t bit_index) {
	*get_bitarray_chunk(bitarray, bit_index) |= get_mask_for_bit_index_in_chunk(bit_index);
}

bool bitarray_bit_is_set(const BitArray bitarray, const buffer_size_t bit_index) {
	return CHECK_BITMASK(*get_bitarray_chunk(bitarray, bit_index), get_mask_for_bit_index_in_chunk(bit_index));
}
