#include <x86intrin.h>
#include <stdio.h>

typedef __m128i color4_t; // a, r, g, b

#define init_color4(a, b, c, d) _mm_setr_epi32(d, c, b, a)
#define fill_color4 _mm_set1_epi32
#define color4_ind(c, i) _mm_extract_epi32(c, 3 - i)
#define print_color4(c) printf("%d, %d, %d, %d\n", color4_ind(c, 0), color4_ind(c, 1), color4_ind(c, 2), color4_ind(c, 3))

int main(void) {
	const color4_t foo = init_color4(1, 2, 3, 4), bar = init_color4(5, 6, 7, 8);
	const color4_t baz = bar * foo;
	print_color4(baz);
}
