inlinable vec vec_tex_offset(const vec pos, const int tex_size) {
	return vec_fill(tex_size) * (pos - vec_trunc(pos));
}

inlinable Uint32* read_texture_row(const void* const pixels, const int pixel_pitch, const int y) {
	return (Uint32*) ((Uint8*) pixels + y * pixel_pitch);
}

#ifdef SHADING_ENABLED

/*
inlinable __m128i _mm_div_epi32(const __m128i a, const __m128i b) {
	return _mm_setr_epi32(a[0] / b[0], a[1] / b[1], a[2] / b[2], a[3] / b[3]);
}
*/

inlinable Uint32 shade_ARGB_pixel(const Uint32 pixel, const byte shade) {
	/* const __m128i colors = _mm_setr_epi32(0, (byte) pixel, (byte) (pixel >> 8), (byte) (pixel >> 16));
	const __m128i first_part_shaded = _mm_mul_epi32(colors, _mm_set1_epi16(shade));
	const __m128i divided = _mm_div_epi32(first_part_shaded, _mm_set1_epi16(255)); // why no _mm_div_epi32 instruction?
	return 0xFF000000 | (_mm_extract_epi32(divided, 0) << 16) | (_mm_extract_epi32(divided, 1) << 8) | divided[2]; */

	unsigned r = (byte) (pixel >> 16), g = (byte) (pixel >> 8), b = (byte) pixel;

	r *= shade;
	g *= shade;
	b *= shade;

	r /= 255;
	g /= 255;
	b /= 255;

	return 0xFF000000 | (r << 16) | (g << 8) | b;
}

#endif

static PixSprite ground;
inlinable Uint32 get_pixel_from_hit(const vec hit, const double dist) {
	const vec offset = vec_tex_offset(hit, ground.size);

	Uint32 pixel = ground.pixels[(long) ((long) offset[1] * ground.size + offset[0])];

	#ifdef SHADING_ENABLED
	pixel = shade_ARGB_pixel(pixel, calculate_shade(settings.proj_dist / dist, hit));
	#else
	(void) dist;
	#endif

	return pixel;
}

void fast_affine_floor(const byte floor_height, const vec pos,
	const double p_height, const double pace, double y_shift, const int y_pitch) {

	const double screen_height_proj_ratio = settings.screen_height / settings.proj_dist;
	const double world_height = p_height - floor_height / screen_height_proj_ratio;

	// if the player is under the floor plane
	if (world_height < settings.plane_bottom) return;

	const double opp_h = 0.5 + world_height * screen_height_proj_ratio;

	if (y_shift < 0.0) y_shift = 0.0;
	for (int y = y_shift - pace; y < settings.screen_height - pace; y++) {
		const int row = y - settings.half_screen_height - y_pitch + 1;
		if (row == 0) continue;

		const double straight_dist = opp_h / row * settings.proj_dist;

		const int pace_y = y + pace;
		Uint32* const pixbuf_row = read_texture_row(screen.pixels, screen.pixel_pitch, pace_y);

		for (int screen_x = 0; screen_x < settings.screen_width; screen_x++) {
			if (get_statemap_bit(occluded_by_walls, screen_x, pace_y)) continue;

			/* The remaining bottlenecks:
			Checking for statemap bits will not be needed with a visplane system -> speedup
			Also, that means that the statemap won't be needed anymore -> speedup
			Checking for out-of-bound hits won't be needed either -> speedup
			Checking for wall points that do not equal the floor height won't be needed either -> speedup
			Also, with a visplane system, less y and x coordinates will be iterated over -> speedup */

			const BufferVal buffer_val = val_buffer[screen_x];
			const double actual_dist = straight_dist * (double) buffer_val.one_over_cos_beta;
			const vec hit = vec_line_pos(pos, buffer_val.dir, actual_dist);

			if (hit[0] < 1.0 || hit[1] < 1.0 || hit[0] > current_level.map_size.x - 1.0
				|| hit[1] > current_level.map_size.y - 1.0) continue;

			#ifndef PLANAR_MODE
			const byte wall_point = map_point(current_level.wall_data, hit[0], hit[1]);
			if (current_level.get_point_height(wall_point, hit) != floor_height) continue;
			#endif

			const Uint32 pixel = get_pixel_from_hit(hit, actual_dist);
			for (byte x = 0; x < settings.ray_column_width; x++) pixbuf_row[x + screen_x] = pixel;
		}
	}
}

#ifdef PLANAR_MODE

void fill_val_buffers_for_planar_mode(const double angle_degrees) {
	const double p_angle = to_radians(angle_degrees);

	for (int screen_x = 0; screen_x < settings.screen_width; screen_x += settings.ray_column_width) {
		const double theta = atan((screen_x - settings.half_screen_width) / settings.proj_dist) + p_angle;
		update_val_buffer(screen_x, 0.0f, cosf(p_angle - theta), (vec) {cos(theta), sin(theta)});
	}
}

#endif

void draw_colored_floor(const double y_shift) {
	const SDL_FRect floor = {0.0, y_shift - 1.0, settings.screen_width, settings.screen_height - y_shift + 1.0};
	SDL_SetRenderDrawColor(screen.renderer, 148, 107, 69, SDL_ALPHA_OPAQUE);
	SDL_RenderFillRectF(screen.renderer, &floor);
}
