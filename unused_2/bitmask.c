#include <stdint.h>
#include <stdio.h>

typedef uint_fast8_t byte;

const byte sixth_bit_set = 0b01000000;

int main(void) {
	byte bits = 0b10111111;

	bits |= sixth_bit_set;

	printf("%d\n", !!(bits & sixth_bit_set));
}
