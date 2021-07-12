typedef struct {
	double dist;
	byte step_count, side;
	const double origin[2], dir[2], unit_step_size[2];
	double ray_length[2];
	const ivec ray_step;
	ivec curr_tile;
} DataDDA;

DataDDA init_dda(const vec origin, const vec dir) {
	const double unit_step_size[2] = {fabs(1.0 / dir[0]), fabs(1.0 / dir[1])};
	double ray_length[2];
	const ivec curr_tile = vec_to_ivec(origin);
	ivec ray_step;

	if (dir[0] < 0.0) {
		ray_step.x = -1;
		ray_length[0] = (origin[0] - curr_tile.x) * unit_step_size[0];
	}
	else {
		ray_step.x = 1;
		ray_length[0] = (curr_tile.x + 1.0 - origin[0]) * unit_step_size[0];
	}

	if (dir[1] < 0.0) {
		ray_step.y = -1;
		ray_length[1] = (origin[1] - curr_tile.y) * unit_step_size[1];
	}
	else {
		ray_step.y = 1;
		ray_length[1] = (curr_tile.y + 1.0 - origin[1]) * unit_step_size[1];
	}

	// origin and dir are braced b/c vec -> double[2], and the arrays can't be copied directly
	DataDDA dda_data = {
		0.0, 0, 0, {origin[0], origin[1]}, {dir[0], dir[1]},
		{unit_step_size[0], unit_step_size[1]}, {ray_length[0], ray_length[1]},
		ray_step, curr_tile
	};

	return dda_data;
}

inlinable DataDDA peek_dda(DataDDA d) {
	if (d.ray_length[0] < d.ray_length[1]) {
		d.dist = d.ray_length[0];
		d.curr_tile.x += d.ray_step.x;
		d.ray_length[0] += d.unit_step_size[0];
		d.side = 0;
	}
	else {
		d.dist = d.ray_length[1];
		d.curr_tile.y += d.ray_step.y;
		d.ray_length[1] += d.unit_step_size[1];
		d.side = 1;
	}

	return d;
}

inlinable byte iter_dda(DataDDA* const d_ref) {
	DataDDA d = peek_dda(*d_ref);

	if (ivec_out_of_bounds(d.curr_tile)) return 0;

	d.step_count++;
	memcpy(d_ref, &d, sizeof(DataDDA));
	return 1;
}
