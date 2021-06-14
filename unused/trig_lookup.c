typedef struct {
	int fn_type, num_vals;
	double* vals, step;
} TrigTable;

#define deinit_trig_table(table) free(table.vals);

const TrigTable init_trig_table(const int fn_type, const int num_vals) {
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

inlinable const double _lookup(const TrigTable table, const double x) {
	return table.vals[(int) round(x / table.step)];
}

inlinable const double lookup_sin(double x) {
	return sin(x);

	const double orig_x = x;
	if (x < 0) x = -x;
	if (x > two_pi) x = fmod(x, two_pi);

	const double result = _lookup(sin_table, x);
	return orig_x < 0 ? -result : result;
}

inlinable const double lookup_cos(double x) {
	return lookup_sin(half_pi - x);
}

inlinable double lookup_tan(double x) {
	return lookup_sin(x) / lookup_cos(x);
}
