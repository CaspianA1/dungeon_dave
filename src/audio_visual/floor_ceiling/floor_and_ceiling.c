inlinable Uint8 get_bits(const Uint32 value, const Uint8 offset, const Uint8 n) {
	return (value >> offset) & ((1 << n) - 1);
}

#ifdef SHADING_ENABLED

inlinable Uint32 shade_ARGB_pixel(const Uint32 pixel, const double dist, const VectorF pos) {

	#ifndef SHADING_ENABLED
	(void) dist;
	(void) pos;
	#endif

	/* why isn't (a | b | c) * d equal to ((a * d) | (b * d) | (c * d))?
	settings.proj_dist / dist is the shading of an imaginary wall */
	const double shade = calculate_shade(settings.proj_dist / dist, pos);

	const Uint8 // this could perhaps use some SIMD
		r = get_bits(pixel, 16, 23) * shade,
		g = get_bits(pixel, 8, 15) * shade,
		b = get_bits(pixel, 0, 8) * shade;

	return 0xFF000000 | (r << 16) | (g << 8) | b;
}

#else

#define shade_ARGB_pixel(pixel, a, b) pixel

#endif

inlinable int get_ceil_row(const double y, const int y_pitch) {
	return settings.half_screen_height + y_pitch - y;
}

inlinable int get_floor_row(const double y, const int y_pitch) {
	return y - settings.half_screen_height - y_pitch + 1;
}

void draw_floor_or_ceil(
	const VectorF pos, const VectorF dir, 
	const byte is_ceiling, const int screen_x, const double begin, const double end,
	const double cos_beta, const double pace, const int y_pitch, const double p_height) {

	if (end <= begin) return;

	byte* tex_hit_data;
	int (*get_row) (const double, const int);

	if (is_ceiling) {
		tex_hit_data = current_level.ceiling_data;
		get_row = get_ceil_row;
	}

	else {
		tex_hit_data = current_level.floor_data;
		get_row = get_floor_row;
	}

	// I'm dividing and multiplying by proj dist, could I eliminate that?
	const double p_height_ratio = p_height * settings.screen_height / settings.proj_dist;
	const double opp_h = 0.5 + (is_ceiling ? -p_height_ratio : p_height_ratio);

	for (int y = begin - pace; y < end - pace; y++) {
		const int pace_y = y + pace;
		if (pace_y < 0 || pace_y >= settings.screen_height) continue;

		const double straight_dist = opp_h / get_row(y, y_pitch) * settings.proj_dist;
		const double actual_dist = straight_dist / cos_beta;

		const VectorF hit = VectorF_line_pos(pos, dir, actual_dist);

		if (hit[0] <= 1.0 || hit[0] >= current_level.map_width - 1.0
			|| hit[1] <= 1.0 || hit[1] >= current_level.map_height - 1.0)
			continue;

		const VectorI floored_hit = VectorF_floor(hit);
		const byte point = map_point(tex_hit_data, floored_hit.x, floored_hit.y);
		const SDL_Surface* const surface = current_level.walls[point - 1].surface;
		const int max_offset = surface -> w - 1;

		const VectorI surface_offset = {
			(hit[0] - floored_hit.x) * max_offset,
			(hit[1] - floored_hit.y) * max_offset
		};

		// bottleneck
		Uint32 surface_pixel = get_surface_pixel(surface -> pixels, surface -> pitch,
			surface_offset.x, surface_offset.y);

		// bottleneck
		surface_pixel = shade_ARGB_pixel(surface_pixel, actual_dist, hit);

		for (int x = screen_x; x < screen_x + settings.ray_column_width; x++)
			*get_pixbuf_pixel(x, pace_y) = surface_pixel;
	}
}

void simd_draw_floor_or_ceil(const VectorF, const VectorF, 
	const byte, const int, const double, const double,
	const double, const double, const int, const double);

inlinable void std_draw_ceiling(const VectorF pos, const VectorF dir,
	const double pace, const int y_pitch, const double p_height,
	const double cos_beta, const SDL_FRect wall)  {

	simd_draw_floor_or_ceil(pos, dir, 1, wall.x, 0.0, (double) wall.y + 1.0,
		cos_beta, pace, y_pitch, p_height);
}

// inlinable void std_draw_floor(const Player player, const VectorF dir,
inlinable void std_draw_floor(const VectorF pos, const VectorF dir,
	const double pace, const int y_pitch, const double p_height,
	const double cos_beta, const SDL_FRect wall)  {

	simd_draw_floor_or_ceil(pos, dir, 0, wall.x, (double) (wall.y + wall.h),
		settings.screen_height, cos_beta, pace, y_pitch, p_height);
}
