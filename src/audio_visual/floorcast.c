inlinable byte get_bits(const Uint32 value, const byte offset, const byte n) {
	return (value >> offset) & ((1 << n) - 1);
}

inlinable void draw_from_hit(const VectorF hit, const double actual_dist, const int screen_x, const int pace_y) {
	const VectorI floored_hit = VectorF_floor(hit);

	const byte point = map_point(current_level.floor_data, floored_hit.x, floored_hit.y);
	const SDL_Surface* const surface = current_level.walls[point - 1].surface;
	const int max_offset = surface -> w - 1;

	const VectorI surface_offset = {
		(hit[0] - floored_hit.x) * max_offset,
		(hit[1] - floored_hit.y) * max_offset
	};

	Uint32 src = get_surface_pixel(surface -> pixels, surface -> pitch, surface_offset.x, surface_offset.y);

	#ifdef SHADING_ENABLED
	const double shade = calculate_shade(settings.proj_dist / actual_dist, hit);
	const byte r = get_bits(src, 16, 23) * shade, g = get_bits(src, 8, 15) * shade, b = get_bits(src, 0, 8) * shade;
	src = 0xFF000000 | (r << 16) | (g << 8) | b;
	#else
	(void) actual_dist;
	#endif

	for (int x = screen_x; x < screen_x + settings.ray_column_width; x++)
		*get_pixbuf_pixel(x, pace_y) = src;
}

void fast_affine_floor(const VectorF pos, const double p_height, const double pace, const int y_pitch, const double y_shift) {
	const double p_height_ratio = p_height * settings.screen_height / settings.proj_dist;
	const double opp_h = 0.5 + p_height_ratio;

	// y_shift - pace may go outside map boundaries; limit this domain
	for (int y = y_shift - pace; y < settings.screen_height - pace; y++) {
		const int pace_y = y + pace;
		if (pace_y < 0) continue;

		const int row = y - settings.half_screen_height - y_pitch + 1;
		if (row == 0) continue;
		const double straight_dist = opp_h / row * settings.proj_dist;

		for (int screen_x = 0; screen_x < settings.screen_width; screen_x += settings.ray_column_width) {
			const double actual_dist = straight_dist / screen.cos_beta_buffer[screen_x];
			const VectorF hit = VectorFF_add(VectorFF_mul(screen.dir_buffer[screen_x], VectorF_memset(actual_dist)), pos);

			if (hit[0] < 0.0 || hit[1] < 0.0 || hit[0] > current_level.map_width - 1.0
				|| hit[1] > current_level.map_height - 1.0) continue;
			draw_from_hit(hit, actual_dist, screen_x, pace_y);
		}
	}
}
