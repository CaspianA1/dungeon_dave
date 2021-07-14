#include <stdint.h>

const double shade = 0.8;

typedef uint_fast8_t byte;

uint32_t orig_shade(const uint32_t pixel) {
	const byte r = (byte) (pixel >> 16) * shade, g = (byte) (pixel >> 8) * shade, b = (byte) pixel * shade;
	return 0xFF000000 | (r << 16) | (g << 8) | b;
}

uint32_2 shade_2(const uint32_t pixel) {
}

int main(void) {

}
