typedef struct {
	byte side;
	double dist, ray_length_components[2];
	const double unit_step_size[2];
	const ivec ray_step;
	ivec curr_tile;
} DataDDA;

DataDDA init_dda(const vec origin, const vec dir) {
	double ray_length_components[2];
	const double unit_step_size[2] = {fabs(1.0 / dir[0]), fabs(1.0 / dir[1])};

	ivec ray_step;
	const ivec curr_tile = ivec_from_vec(origin);

	if (dir[0] < 0.0) {
		ray_step.x = -1;
		ray_length_components[0] = (origin[0] - curr_tile.x) * unit_step_size[0];
	}
	else {
		ray_step.x = 1;
		ray_length_components[0] = (curr_tile.x + 1.0 - origin[0]) * unit_step_size[0];
	}

	if (dir[1] < 0.0) {
		ray_step.y = -1;
		ray_length_components[1] = (origin[1] - curr_tile.y) * unit_step_size[1];
	}
	else {
		ray_step.y = 1;
		ray_length_components[1] = (curr_tile.y + 1.0 - origin[1]) * unit_step_size[1];
	}

	// origin and dir are braced b/c vec -> double[2], and the arrays can't be copied directly
	return (DataDDA) {
		0, 0.0, {ray_length_components[0], ray_length_components[1]},
		{unit_step_size[0], unit_step_size[1]}, ray_step, curr_tile
	};
}

inlinable DataDDA peek_dda(DataDDA d) {
	d.side = d.ray_length_components[0] >= d.ray_length_components[1];

	d.dist = d.ray_length_components[d.side];
	*(d.side ? &d.curr_tile.y : &d.curr_tile.x) += (d.side ? d.ray_step.y : d.ray_step.x);

	d.ray_length_components[d.side] += d.unit_step_size[d.side];

	return d;
}

inlinable byte iter_dda(DataDDA* const d_ref) {
	DataDDA d = peek_dda(*d_ref);
	if (ivec_out_of_bounds(d.curr_tile)) return 0;

	// d.step_count++;
	memcpy(d_ref, &d, sizeof(DataDDA));
	return 1;
}
