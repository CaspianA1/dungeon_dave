/* A statemap is just a matrix of bits. I didn't call it
a bitmat because that sounds too much like bitmap. */

#define clear_statemap(s) memset(s.data, 0, s.alloc_bytes)
#define deinit_statemap(s) wfree(s.data);

StateMap init_statemap(const int bits_across, const int bits_down) {
	const int across = ceil(bits_across / 8.0);

	StateMap statemap = {
		{across, bits_down},
		.alloc_bytes = across * bits_down
	};

	statemap.data = wmalloc(statemap.alloc_bytes);
	clear_statemap(statemap);

	return statemap;
}

//////////

inlinable byte* statemap_byte(const StateMap statemap, const int x, const int y) {
	return statemap.data + (y * statemap.chunk_dimensions.x + x);
}

inlinable byte get_n_for_bits_x(const int bits_x) {
	return 7 - (bits_x & 7); // bits_x & 7 == bits_x % 8
}

// returns if bit was previously set
inlinable byte set_statemap_bit_with_status(const StateMap statemap, const int bits_x, const int bits_y) {
	byte* const bits = statemap_byte(statemap, bits_x / 8, bits_y);
	const byte n = get_n_for_bits_x(bits_x);
	const byte was_set = (*bits >> n) & 1; // if bit was set
	*bits |= 1 << n; // sets nth bit
	return was_set;
}

void set_statemap_bit(const StateMap statemap, const int bits_x, const int bits_y) {
	*statemap_byte(statemap, bits_x / 8, bits_y) |= 1 << get_n_for_bits_x(bits_x);
}

inlinable byte get_statemap_bit(const StateMap statemap, const int bits_x, const int bits_y) {
	return *statemap_byte(statemap, bits_x / 8, bits_y) >> (get_n_for_bits_x(bits_x) & 1);
}

/*
static void print_statemap(const StateMap statemap) {
	for (int y = 0; y < statemap.chunk_dimensions.y; y++) {
		putchar('|');
		for (int x = 0; x < statemap.chunk_dimensions.x; x++) {
			const byte bits = *statemap_byte(statemap, x, y);
			for (int i = 7; i >= 0; i--) printf("%c", (bits & (1 << i)) ? '1' : '0');
			putchar('|');
		}
		putchar('\n');
	}
}
*/
