#ifndef STATEMAP_C
#define STATEMAP_C

#include "headers/statemap.h"

/* A statemap is just a matrix of bits. I didn't call it
a bitmat because that sounds too much like bitmap. */

StateMap init_statemap(const unsigned bits_across, const unsigned bits_down) {
	const unsigned across = (unsigned) ceil(bits_across / 8.0);

	StateMap statemap = {
		{across, bits_down},
		.alloc_bytes = across * bits_down
	};

	statemap.data = malloc(statemap.alloc_bytes);
	clear_statemap(statemap);

	return statemap;
}

static byte* statemap_byte(const StateMap statemap, const unsigned chunk_x, const unsigned y) {
	return statemap.data + (y * statemap.chunk_dimensions[0] + chunk_x);
}

/* For an x-offset in the statemap, this returns the bit offset
that should be set or returned in a function below */
static byte get_n_for_bits_x(const unsigned bits_x) {
	return 7u - (bits_x & 7u); // bits_x & 7 == bits_x % 8
}

void set_statemap_bit(const StateMap statemap, const unsigned bits_x, const unsigned bits_y) {
	byte* const bits = statemap_byte(statemap, bits_x >> 3u, bits_y);
	set_bit(*bits, 1u << get_n_for_bits_x(bits_x));
}

byte get_statemap_bit(const StateMap statemap, const unsigned bits_x, const unsigned bits_y) {
	const byte smb = *statemap_byte(statemap, bits_x >> 3u, bits_y);
	return bit_is_set(smb, 1u << get_n_for_bits_x(bits_x));
}

#endif
