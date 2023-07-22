#include "utils/bitarray.h"

void set_bit_range_in_bitarray(const BitArray bitarray, const buffer_size_t start, const buffer_size_t end) {
	bitarray_chunk_t
		*const first_chunk = get_bitarray_chunk(bitarray, start),
		*const last_chunk = get_bitarray_chunk(bitarray, end);

	static const bitarray_chunk_t chunk_all_bits_set_except_for_first = (bitarray_chunk_t) ~(bitarray_chunk_t) 1u;

	const bitarray_chunk_t
		first_mask = (bitarray_chunk_t) (chunk_with_all_bits_set << (start & bits_per_chunk_minus_one)),
		last_mask = (bitarray_chunk_t) ~(chunk_all_bits_set_except_for_first << (end & bits_per_chunk_minus_one));

	//////////

	if (first_chunk == last_chunk)
		*first_chunk |= first_mask & last_mask;
	else {
		*first_chunk |= first_mask;
		*last_chunk |= last_mask;

		for (bitarray_chunk_t* chunk = first_chunk + 1; chunk < last_chunk; chunk++)
			*chunk = chunk_with_all_bits_set;
	}
}
