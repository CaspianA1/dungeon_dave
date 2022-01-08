/* A statemap is just a matrix of bits. I didn't call it
a bitmat because that sounds too much like bitmap. */

#define clear_statemap(s) memset(s.data, 0, s.alloc_bytes)
#define deinit_statemap(s) wfree(s.data);

StateMap init_statemap(const unsigned bits_across, const unsigned bits_down) {
	const unsigned across = ceil(bits_across / 8.0);

	StateMap statemap = {
		{across, bits_down},
		.alloc_bytes = across * bits_down
	};

	statemap.data = wmalloc(statemap.alloc_bytes);
	clear_statemap(statemap);

	return statemap;
}

//////////

inlinable byte* statemap_byte(const StateMap statemap, const unsigned chunk_x, const unsigned y) {
	return statemap.data + (y * statemap.chunk_dimensions[0] + chunk_x);
}

/* For an x-offset in the statemap, this returns the bit offset
that should be set or returned in a function below */
inlinable byte get_n_for_bits_x(const unsigned bits_x) {
	return 7u - (bits_x & 7u); // bits_x & 7 == bits_x % 8
}

// Returns if a bit was previously set + sets a bit all in one
inlinable byte set_statemap_bit_with_status(const StateMap statemap, const unsigned bits_x, const unsigned bits_y) {
	byte* const bits = statemap_byte(statemap, bits_x >> 3u, bits_y); // shr 3 = div 8
	const byte n_mask = 1u << get_n_for_bits_x(bits_x);
	const byte was_set = bit_is_set(*bits, n_mask);
	set_bit(*bits, n_mask);
	return was_set;
}

inlinable void set_statemap_bit(const StateMap statemap, const unsigned bits_x, const unsigned bits_y) {
	byte* const bits = statemap_byte(statemap, bits_x >> 3u, bits_y);
	set_bit(*bits, 1u << get_n_for_bits_x(bits_x));
}

inlinable byte get_statemap_bit(const StateMap statemap, const unsigned bits_x, const unsigned bits_y) {
	const byte smb = *statemap_byte(statemap, bits_x >> 3u, bits_y);
	return bit_is_set(smb, 1u << get_n_for_bits_x(bits_x));
}

// TODO: to static
void print_statemap(const StateMap statemap) {
	for (unsigned y = 0; y < statemap.chunk_dimensions[1]; y++) {
		putchar('|');
		for (unsigned x = 0; x < statemap.chunk_dimensions[0]; x++) {
			const byte bits = *statemap_byte(statemap, x, y);
			for (int8_t i = 7; i >= 0; i--) putchar(((bits >> i) & 1) + '0');
			putchar('|');
		}
		putchar('\n');
	}
}
