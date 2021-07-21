/* a statemap is just a matrix of bits. I didn't call it
a bitmat because that sounds too much like bitmap. */

typedef struct {
	const ivec byte_dimensions;
	byte* data;
} StateMap;

StateMap init_statemap(const int bits_across, const int bits_down) {
	StateMap statemap = {
		.byte_dimensions = {ceil(bits_across / 8.0), ceil(bits_down / 8.0)}
	};

	const int total_bytes = statemap.byte_dimensions.x * statemap.byte_dimensions.y;
	statemap.data = malloc(total_bytes);
	memset(statemap.data, 0, total_bytes);

	return statemap;
}

//////////

byte* statemap_byte(const StateMap statemap, const int x, const int y) {
	return statemap.data + (y * statemap.byte_dimensions.x + x);
}

void print_statemap(const StateMap statemap) {
	const ivec byte_dimensions = statemap.byte_dimensions;

	*statemap_byte(statemap, 0, 0) = 0b11111111;
	// next: set bits on a bit scale

	for (int y = 0; y < byte_dimensions.y; y++) {
		putchar('|');
		for (int x = 0; x < byte_dimensions.x; x++) {
			const byte bits = *statemap_byte(statemap, x, y);
			for (int i = 7; i >= 0; i--) printf("%c", (bits & (1 << i)) ? '1' : '0');
			putchar('|');
		}
		putchar('\n');
	}
}

void statemap_test(void) {
	const StateMap statemap = init_statemap(16, 16); 
	print_statemap(statemap);

	exit(0);
}
