inlinable void draw_from_hit(const VectorF hit, const double actual_dist, const int screen_x, Uint32* const pixbuf_row) {
	const VectorI floored_hit = VectorF_floor(hit);
	const byte point = map_point(current_level.floor_data, floored_hit.x, floored_hit.y);

	// why is the point height never zero?
	// if (current_level.get_point_height(point, hit) != 0) return;

	const Sprite sprite = current_level.walls[point - 1];
	const SDL_Surface* const surface = sprite.surface;
	const int max_offset = surface -> w - 1;

	const VectorI surface_offset = {
		(hit[0] - floored_hit.x) * max_offset,
		(hit[1] - floored_hit.y) * max_offset
	};

	/*
	Uint32* const src_row = (Uint32*) ((Uint8*) texture -> pixels + surface_offset.y * screen.pixel_pitch);
	Uint32 src = *(src_row + surface_offset.x);
	*/

	Uint32 src = get_surface_pixel(surface -> pixels, surface -> pitch, surface_offset.x, surface_offset.y);

	#ifdef SHADING_ENABLED
	const double shade = calculate_shade(settings.proj_dist / actual_dist, hit);
	const byte r = (byte) (src >> 16) * shade, g = (byte) (src >> 8) * shade, b = (byte) src * shade;
	src = 0xFF000000 | (r << 16) | (g << 8) | b;

	#else
	(void) actual_dist;
	#endif

	for (int x = screen_x; x < screen_x + settings.ray_column_width; x++)
		*(pixbuf_row + x) = src;
}

void fast_affine_floor(const VectorF pos, const double full_jump_height,
	const double pace, double y_shift, const int y_pitch) {

	const double opp_h = 0.5 + full_jump_height / settings.proj_dist;
	if (y_shift < 0.0) y_shift = 0.0;

	// `y_shift - pace` may go outside the map boundaries; limit this domain
	for (int y = y_shift - pace; y < settings.screen_height - pace; y++) {
		const int pace_y = y + pace;

		const int row = y - settings.half_screen_height - y_pitch + 1;
		if (row == 0) continue;
		const double straight_dist = opp_h / row * settings.proj_dist;

		Uint32* const pixbuf_row = (Uint32*) ((Uint8*) screen.pixels + pace_y * screen.pixel_pitch);

		for (int screen_x = 0; screen_x < settings.screen_width; screen_x += settings.ray_column_width) {
			if (screen.wall_bottom_buffer[screen_x] >= pace_y + 1) continue;
			// factor in point heights for the area after the stairs to skip more rendering

			const double actual_dist = straight_dist / screen.cos_beta_buffer[screen_x];
			const VectorF hit = VectorFF_add(VectorFF_mul(screen.dir_buffer[screen_x], VectorF_memset(actual_dist)), pos);

			draw_from_hit(hit, actual_dist, screen_x, pixbuf_row);
		}
	}
}

#ifdef PLANAR_MODE

void fill_val_buffers_for_planar_mode(const double angle_degrees) {
	const double player_angle = to_radians(angle_degrees);

	for (int screen_x = 0; screen_x < settings.screen_width; screen_x += settings.ray_column_width) {
		const double theta = atan((screen_x - settings.half_screen_width) / settings.proj_dist) + player_angle;
		update_val_buffers(screen_x, 0.0, cos(player_angle - theta), 0.0f, (VectorF) {cos(theta), sin(theta)});
	}
}

#endif
