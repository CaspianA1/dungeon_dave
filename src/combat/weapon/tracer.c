// Tracing is separate from DDA because DDA inherently steps on whole grids, while tracers do not

static const float
	short_range_tracer_step = 0.3f, // the magnitude of the velocity vector
	long_range_tracer_step = 0.1f,
	long_range_projectile_tracer_step = 0.1f,
	hitscan_projectile_size = 0.2f,
	inter_tick_projectile_size = 1.0f;

inlinable Tracer init_tracer_from_player(const Player* const player, const float step, const byte is_hitscan) {
	const vec p_pos = player -> pos, p_dir = player -> dir; // these are 2D

	return (Tracer) { // Shoots from center of player
		{p_pos[0], p_pos[1], player -> jump.height + actor_eye_height},
		{p_dir[0], p_dir[1], atan((player -> y_pitch + player -> pace.screen_offset) / settings.proj_dist)},
		0.0, step, is_hitscan
	};
}

// Returns if tracing should continue
inlinable byte iter_tracer(Tracer* const tracer) {
	const float step = tracer -> step;

	tracer -> dist += step;
	const vec3D new_pos = tracer -> pos + tracer -> dir * vec_fill_3D(step);
	tracer -> pos = new_pos;

	double height = (double) new_pos[2];
	if (!tracer -> is_hitscan) height -= (double) inter_tick_projectile_size * 0.5;

	return !point_exists_at((double) new_pos[0], (double) new_pos[1], height);
}
