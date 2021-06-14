#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#define inlinable extern inline

typedef struct {
	int fn_type, num_vals;
	double* vals, step;
} TrigTable;

enum {trig_precision = 15000, trig_max = 100};
const double two_pi = 2 * M_PI, half_pi = M_PI / 2, trig_step = 0.00001;
TrigTable sin_table;

#include "../src/trig_lookup.c"

int64_t millis() {
	struct timespec now;
	timespec_get(&now, TIME_UTC);
	return ((int64_t) now.tv_sec) * 1000 + ((int64_t) now.tv_nsec) / 1000000;
}

const int64_t benchmark(double (*trig_fn) (double)) {
	const int64_t before = millis();

	for (double i = 0; i < trig_max; i += trig_step)
		trig_fn(i);

	return millis() - before;
}

int main() {
	sin_table = init_trig_table(0, trig_precision);

	int64_t tabled = benchmark(lookup_tan), untabled = benchmark(tan);

	printf("Tabled vs not: %lld ms, %lld ms\n", tabled, untabled);

	deinit_trig_table(sin_table);
}