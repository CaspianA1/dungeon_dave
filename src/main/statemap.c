/* A statemap is just a matrix of bits. I didn't call it
a bitmat because that sounds too much like bitmap. */

typedef struct {
	const ivec chunk_dimensions;
	const int alloc_bytes;
	byte* data;
} StateMap;

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

byte* statemap_byte(const StateMap statemap, const int x, const int y) {
	return statemap.data + (y * statemap.chunk_dimensions.x + x);
}

void set_nth_bit(byte* const bits, const byte n) {
	*bits |= 1 << (7 - n);
}

void set_statemap_bit(const StateMap statemap, const int bits_x, const int bits_y) {
	byte* const bits = statemap_byte(statemap, bits_x / 8, bits_y);
	set_nth_bit(bits, bits_x % 8);
}

void print_statemap(const StateMap statemap) {
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

