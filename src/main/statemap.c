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
	memset(statemap.data, 0, statemap.alloc_bytes);

	return statemap;
}

//////////

inlinable byte* statemap_byte(const StateMap statemap, const int x, const int y) {
	return &statemap.data[y * statemap.chunk_dimensions.x + x];
}

inlinable void set_nth_bit(byte* const bits, const byte n) {
	*bits |= 1 << n;
}

// not working yet
void set_statemap_bit(const StateMap statemap, const int bits_x, const int bits_y) {
	(void) statemap;

	const ivec byte_coords = {bits_x / 8.0, bits_y};
	DEBUG_IVEC(byte_coords);
	byte* const bits = statemap_byte(statemap, byte_coords.x, byte_coords.y);
	DEBUG(*bits, d);
	// set_nth_bit(bits, bits_x % 8);
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

void statemap_test(void) {
	const StateMap statemap = init_statemap(32, 32); // 32 * 32 bits = 1024 bits = 256 bytes
	set_statemap_bit(statemap, 1, 1);
	// *statemap_byte(statemap, 1, 1) = 0b11111111;

	print_statemap(statemap);

	exit(0);
}
