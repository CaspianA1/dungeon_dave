typedef struct {
	byte step_count, side;
	double dist;
	const double origin[2], dir[2], unit_step_size[2], ray_step[2];
	double ray_length[2], curr_tile[2];
} DataDDA;

DataDDA init_dda(const vec origin, const vec dir, const double step) {
	const double unit_step_size[2] = {fabs(step / dir[0]), fabs(step / dir[1])};
	double ray_length[2];
	const vec curr_tile = {floor(origin[0]), floor(origin[1])};
	double ray_step[2];

	if (dir[0] < 0.0) {
		ray_step[0] = -step;
		ray_length[0] = (origin[0] - curr_tile[0]) * unit_step_size[0];
	}
	else {
		ray_step[0] = step;
		ray_length[0] = (curr_tile[0] + step - origin[0]) * unit_step_size[0];
	}

	if (dir[1] < 0.0) {
		ray_step[1] = -step;
		ray_length[1] = (origin[1] - curr_tile[1]) * unit_step_size[1];
	}
	else {
		ray_step[1] = step;
		ray_length[1] = (curr_tile[1] + step - origin[1]) * unit_step_size[1];
	}

	// origin and dir are braced b/c vec -> double[2], and the arrays can't be copied directly
	return (DataDDA) {
		0, 0, 0.0, {origin[0], origin[1]}, {dir[0], dir[1]},
		{unit_step_size[0], unit_step_size[1]}, {ray_step[0], ray_step[1]},
		{ray_length[0], ray_length[1]}, {curr_tile[0], curr_tile[1]}
	};
}

inlinable DataDDA peek_dda(DataDDA d) {
	d.side = d.ray_length[0] >= d.ray_length[1];

	d.dist = d.ray_length[d.side];
	d.curr_tile[d.side] += d.ray_step[d.side];
	d.ray_length[d.side] += d.unit_step_size[d.side];

	return d;
}

inlinable byte iter_dda(DataDDA* const d_ref) {
	DataDDA d = peek_dda(*d_ref);
	if (vec_out_of_bounds((vec) {d.curr_tile[0], d.curr_tile[1]})) return 0;

	d.step_count++;
	memcpy(d_ref, &d, sizeof(DataDDA));
	return 1;
}
