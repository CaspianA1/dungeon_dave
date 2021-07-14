#include <stdint.h>
#include <stdio.h>

typedef uint_fast8_t byte;
typedef uint_fast32_t Uint32;

const double shade = 0.8;
const Uint32
	test_pixel =        0b11000100100010001100000010001000,
	second_test_pixel = 0b10001111011111111111111111111001,
	empty_pixel =       0b00000000000000000000000000000000;

void print_bits(const Uint32 val, const char* const msg) { // 30 asm
	printf("%s | ", msg);
	for (Uint32 mask = 0x80000000; mask; mask >>= 1)
		printf("%d", !!(mask & val));
	putchar('\n');
}

Uint32 shade_1(const Uint32 pixel) {
	const byte r = (byte) (pixel >> 16) * shade, g = (byte) (pixel >> 8) * shade, b = (byte) pixel * shade;
	return 0b11111111000000000000000000000000 | (r << 16) | (g << 8) | b;
}

Uint32 shade_2(const Uint32 pixel) { // 13 asm
	/*
	print_bits(pixel, "pixel           ");

	const Uint32 mask = 0b00000000111111111111111111111111;
	print_bits(mask, "mask            ");

	const Uint32 rgb = pixel & mask;
	print_bits(rgb, "rgb             ");

	const Uint32 rgb_shaded = rgb * shade;
	print_bits(rgb_shaded, "rgb shaded      ");

	const Uint32 argb_masked = 0b11111111000000000000000000000000 | rgb_shaded;
	print_bits(argb_masked, "argb masked     ");
	printf("---\n");
	return argb_masked;
	*/
	return 0b11111111000000000000000000000000 | (Uint32) ((pixel & 0b00000000111111111111111111111111) * shade);
}

int main(void) {
	print_bits(shade_2(test_pixel), "quickly shaded  ");
	print_bits(shade_1(test_pixel), "correctly shaded");
	/*
	print_bits(empty_pixel, "empty pixel      ");
	print_bits(test_pixel, "test pixel       ");
	print_bits(second_test_pixel, "second test pixel");
	*/
}
