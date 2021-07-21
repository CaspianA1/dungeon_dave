/* a statemap is just a matrix of bits. I didn't call it
a bitmat because that sounds too much like bitmap.

also, a chunk is a 32-bit number. each chunk can hold 32 states. */

typedef uint32_t statemap_chunk_t; // using _t for scalar types, and pascal case for others

typedef struct {
	ivec chunk_size;
	statemap_chunk_t* const data;
} StateMap;

StateMap init_statemap(const int width, const int height) {
	const ivec dimensions_in_bytes = {
		width / sizeof(statemap_chunk_t),
		height / sizeof(statemap_chunk_t)
	};

	DEBUG_IVEC(dimensions_in_bytes);

	const size_t data_size = dimensions_in_bytes.x * dimensions_in_bytes.y / sizeof(statemap_chunk_t);
	return (StateMap) {dimensions_in_bytes, malloc(data_size)};
}

//////////

void print_bits(const statemap_chunk_t chunk) {
	for (int i = 31; i >= 0; i--)
		printf("%c", (chunk & (1 << i)) ? '1' : '0');
	putchar('\n');
}

void print_statemap(const StateMap statemap) {
	(void) statemap;

	// 00000000000000000000000000000000 = 32 zeros
	print_bits(0b11000000000000000000000000000001);
}

void statemap_test(void) {
	const StateMap statemap = init_statemap(32, 32); // should equal sizeof(uint64_t) * 4
	print_statemap(statemap);

	exit(0);
}
