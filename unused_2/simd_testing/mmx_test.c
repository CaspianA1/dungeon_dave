#include <mmintrin.h>
// /Library/Developer/CommandLineTools/usr/lib/clang/12.0.5/include/mmintrin.h

typedef __m64 vec;
#define short_vec_set _mm_set_pi16
#define vec_set_all _mm_set1_pi16

int main(void) {
	const vec a = short_vec_set(4, 3, 2, 1), b = short_vec_set(9, 8, 7, 8);
	const vec sum = _mm_add_pi16(a, b);

	const vec other = _mm_set_pi32(2.0, 3.0);
}
// whoops - it looks like mmx only works for 1 64-bit int, 2 32-bit ints, 4 16-bit ints, or 8 8-bit ints

// https://stackoverflow.com/questions/2349776/how-can-i-benchmark-c-code-easily
// https://software.intel.com/content/www/us/en/develop/documentation/cpp-compiler-developer-guide-and-reference/top/compiler-reference/intrinsics/intrinsics-for-converting-half-floats/overview-intrinsics-to-convert-half-float-types.html
// https://en.wikipedia.org/wiki/Streaming_SIMD_Extensions
