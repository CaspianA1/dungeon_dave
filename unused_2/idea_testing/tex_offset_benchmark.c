#include <x86intrin.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include <stdio.h>

#define vec_fill _mm_set1_pd
#define inlinable static inline

typedef __m128d vec;
typedef struct {int x, y;} ivec;
typedef unsigned long long bigint;

const bigint iters = 1000000000; // 1 billion
const int tex_size = 64;

/////

inlinable ivec ivec_from_vec(const vec v) {
	return (ivec) {(int) floor(v[0]), (int) floor(v[1])};
}

ivec ivec_tex_offset(const vec pos, const int tex_size) {
	const int max_offset = tex_size - 1;
	const ivec floored_hit = ivec_from_vec(pos);
	return (ivec) {
		(pos[0] - floored_hit.x) * max_offset,
		(pos[1] - floored_hit.y) * max_offset
	};
}

vec vec_tex_offset(const vec pos, const int tex_size) {
	return vec_fill(tex_size - 1) * (pos - _mm_round_pd(pos, _MM_FROUND_TRUNC));
}

ivec benchmark_ivec(void) {
	ivec sum = {0, 0};
	for (bigint i = 0; i < iters; i++) {
		const double pos_x = sin(i) / 3.4;
		const double pos_y = pos_x * i;
		const ivec next_offset = ivec_tex_offset((vec) {pos_x, pos_y}, tex_size);
		sum.x += next_offset.x / pos_x / 3;
		sum.y += next_offset.y / pos_y / 3;
	}
	return sum;
}

vec benchmark_vec(void) {
	vec sum = {0.0, 0.0};
	for (bigint i = 0; i < iters; i++) {
		const double pos_x = sin(i) / 3.4;
		const double pos_y = pos_x * i;
		const vec vec_pos = {pos_x, pos_y};
		sum += vec_tex_offset(vec_pos, tex_size) / vec_pos / vec_fill(3.0);
	}
	return sum;
}

// gcc -Wall -Ofast tex_offset_benchmark.c 
int main(void) {
	clock_t before;
	double delta;

	before = clock();
	ivec result_1 = benchmark_ivec();
	delta = (double) (clock() - before) / CLOCKS_PER_SEC;
	printf("Result 1: {%d, %d}\n", result_1.x, result_1.y);
	printf("Took %lf seconds to complete fn int-based\n", delta);

	/////
	before = clock();
	const vec result_2 = benchmark_vec();
	delta = (double) (clock() - before) / CLOCKS_PER_SEC;
	printf("Result 2: {%lf, %lf}\n", result_2[0], result_2[1]);
	printf("Took %lf seconds to complete fn double-based\n", delta);
}
