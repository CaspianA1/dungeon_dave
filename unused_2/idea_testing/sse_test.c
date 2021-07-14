#include <xmmintrin.h>
#include <time.h>
#include <stdio.h>

/////

typedef __m128d vec;
typedef struct {double x, y;} slow_vec;
typedef unsigned long bigint;

const bigint iters = 1000000000;

#define debug_vec(v) printf(#v " = {%lf, %lf}\n", v[0], v[1])
#define debug_slow_vec(v) printf(#v " = {%lf, %lf}\n", v.x, v.y)

#define slow_vec_add(a, b) ((slow_vec) {a.x + b.x, a.y + b.y})
#define slow_vec_mul(a, b) ((slow_vec) {a.x * b.x, a.y * b.y})
#define slow_vec_div(a, b) ((slow_vec) {a.x / b.x, a.y / b.y})

/////

void vec_fn(void) {
	vec result_1 = {0.0, 0.0};
	for (bigint i = 0; i < iters; i++) {
		const vec a = {3.4, iters / 9.6};
		const vec b = {5.5, iters / a[1]};
		const vec c = a / b + (vec) {3.2, 3.2};
		const vec d = a * b * c;
		result_1 += d;
	}
	debug_vec(result_1);
}

void slow_vec_fn(void) {
	slow_vec result_2 = {0.0, 0.0};
	for (bigint i = 0; i < iters; i++) {
		const slow_vec a = {3.4, iters / 9.6};
		const slow_vec b = {5.5, iters / a.y};
		const slow_vec c = slow_vec_add(slow_vec_div(a, b), ((slow_vec) {3.2, 3.2}));
		const slow_vec d = slow_vec_mul(a, slow_vec_mul(b, c));
		result_2 = slow_vec_add(result_2, d);
	}
	debug_slow_vec(result_2);
}

void benchmark_fn(void (*fn) (), const char* const fn_name) {
	const clock_t before = clock();
	fn();
	const double delta = (double) (clock() - before) / CLOCKS_PER_SEC;
	printf("Took %lf seconds to complete fn %s\n", delta, fn_name);
}

int main(void) {
	benchmark_fn(vec_fn, "simd");
	benchmark_fn(slow_vec_fn, "fake_simd");
}
