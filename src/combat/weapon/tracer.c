// Tracing is separate from DDA because DDA inherently steps on whole grids, while tracers do not

static const float
	short_range_tracer_step = 0.3f, // the magnitude of the velocity vector
	long_range_tracer_step = 0.1f,
	inter_tick_projectile_tracer_step = 0.07f,
	hitscan_projectile_size = 0.2f;

const vec3D inter_tick_projectile_size = {0.5f, 0.5f, 0.34375f}; // 0.34375f = 22 / 64, b/c the sprite is 22 pixels high

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
	const vec3D old_pos = tracer -> pos;

	double old_height = (double) old_pos[2];
	if (!tracer -> is_hitscan) old_height -= (double) inter_tick_projectile_size[2] * 0.5;

	if (point_exists_at((double) old_pos[0], (double) old_pos[1], old_height)) return 0;

	const float step = tracer -> step;
	tracer -> dist += step;
	tracer -> pos += tracer -> dir * vec_fill_3D(step);

	return 1;
}
