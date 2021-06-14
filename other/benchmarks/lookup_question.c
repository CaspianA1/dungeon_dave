#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

const float two_pi = M_PI * 2, half_pi = M_PI / 2;

typedef struct {
	int fn_type, num_vals;
	double* vals, step;
} TrigTable;

static TrigTable sin_table;

TrigTable init_trig_table(const int fn_type, const int num_vals) {
	double (*trig_fn) (double), period;
	switch (fn_type) {
		case 0: trig_fn = sin, period = two_pi; break;
		case 1: trig_fn = cos, period = two_pi; break;
		case 2: trig_fn = tan, period = M_PI; break;
	}

	TrigTable table = {fn_type, num_vals,
		calloc(num_vals, sizeof(double)), period / num_vals};

	for (double x = 0; x < period; x += table.step)
		table.vals[(int) round(x / table.step)] = trig_fn(x);

	return table;
}

double _lookup(const TrigTable table, const double x) {
	return table.vals[(int) round(x / table.step)];
}

double lookup_sin(double x) {
	const double orig_x = x;
	if (x < 0) x = -x;
	if (x > two_pi) x = fmod(x, two_pi);
	// if (x > two_pi) x -= two_pi;

	const double result = _lookup(sin_table, x);
	return orig_x < 0 ? -result : result;
}

double lookup_cos(double x) {
	int scale_factor = 10;
	printf("%d\n", scale_factor);


	// return lookup_sin(half_pi - x);

	/*
	long scaled_x = lround(x * scale_factor);
	scaled_x += number_of_entries/4 ; // If we are doing cosine
	unsigned index = scaled_x & (number_of_entries - 1);  // This & replaces fmod
	double result = table.vals[index];
	return result;
	*/
}

double lookup_tan(double x) {
	return lookup_sin(x) / lookup_cos(x);
}

////////////////////////////////////////

const int64_t millis() {
	struct timespec now;
	timespec_get(&now, TIME_UTC);
	return ((int64_t) now.tv_sec) * 1000 + ((int64_t) now.tv_nsec) / 1000000;
}

const int64_t benchmark(double (*trig_fn) (double)) {
	const int64_t before = millis();

	for (double i = 0; i < 10000; i += 0.001)
		trig_fn(i);

	return millis() - before;
}

int main() {
	sin_table = init_trig_table(0, 4096);

	const int64_t table_time = benchmark(lookup_cos), default_time = benchmark(cos);
	printf("Table time vs default: %lld ms, %lld ms\n", table_time, default_time);

	free(sin_table.vals);
}