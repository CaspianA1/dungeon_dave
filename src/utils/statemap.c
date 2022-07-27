#include "utils/statemap.h"
#include <limits.h>

static const byte bytes_per_chunk = sizeof(statemap_chunk_t);
static const byte bits_per_chunk = CHAR_BIT * bytes_per_chunk;

/*
- A StateMap is just a 2D array of bits. I didn't call it
	a bitmat because that sounds too much like bitmap.

- A chunk is a unit of addressable data in the statemap. Rows
	in the StateMap are composed of these.
*/

StateMap init_statemap(const buffer_size_t bits_across, const buffer_size_t bits_down) {
	// The `ceilf` call rounds up the number of bytes needed for the number of bits across necessary
	const buffer_size_t chunks_across = (buffer_size_t) ceilf((float) bits_across / bits_per_chunk);

	return (StateMap) {
		.chunks_across = chunks_across,
		.data = clearing_alloc(chunks_across * bits_down, bytes_per_chunk)
	};
}

static statemap_chunk_t* get_statemap_chunk(const StateMap statemap,
	const buffer_size_t bits_x, const buffer_size_t bits_y) {

	/* The compiler should make this to a bitwise operation; i.e. `bits_x >> a`,
	where `a` equals the exponent for the power of 2 needed to equal `bits_per_chunk` */
	const buffer_size_t chunk_index = bits_x / bits_per_chunk;
	return statemap.data + (bits_y * statemap.chunks_across + chunk_index);
}

/* Each x-index for a StateMap belongs to a chunk. First, this calculates the bit index within
that chunk; and then, it returns a bitmask where the nth bit is set, where n equals the bit index. */
static statemap_chunk_t get_mask_for_bit_index_in_chunk(const buffer_size_t bits_x) {
	/* The compiler will probably convert this to `bits_x & (bits_per_chunk - 1)`,
	if `bits_per_chunk` is a power of two. */
	const statemap_chunk_t bit_index = bits_x % bits_per_chunk;
	return (statemap_chunk_t) ((statemap_chunk_t) 1u << bit_index);
}

void set_statemap_bit(const StateMap statemap, const buffer_size_t bits_x, const buffer_size_t bits_y) {
	*get_statemap_chunk(statemap, bits_x, bits_y) |= get_mask_for_bit_index_in_chunk(bits_x);
}

// For `area`, [0] and [1] are the origin, and [2] and [3] are the size.
void set_statemap_area(const StateMap statemap, const buffer_size_t area[4]) {
	const buffer_size_t start_x = area[0], start_y = area[1];

	// TODO: make this more efficient by finding a way to set many bits at once across (with an integer type that's as wide as possible)
	for (buffer_size_t y = start_y; y < start_y + area[3]; y++) {
		for (buffer_size_t x = start_x; x < start_x + area[2]; x++)
			set_statemap_bit(statemap, x, y);
	}
}

bool statemap_bit_is_set(const StateMap statemap, const buffer_size_t bits_x, const buffer_size_t bits_y) {
	const statemap_chunk_t chunk = *get_statemap_chunk(statemap, bits_x, bits_y);
	return CHECK_BITMASK(chunk, get_mask_for_bit_index_in_chunk(bits_x));
}
