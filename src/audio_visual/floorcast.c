PSprite p;
inlinable void draw_from_hit(const vec hit, const double actual_dist, const int screen_x, Uint32* const pixbuf_row) {
	const int max_offset = p.size - 1;

	const ivec floored_hit = ivec_from_vec(hit);
	const ivec offset = {
		(hit[0] - floored_hit.x) * max_offset,
		(hit[1] - floored_hit.y) * max_offset
	};

	Uint32 src = *(read_texture_row(p.pixels, p.pitch, offset.y) + offset.x);

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

void fast_affine_floor(const vec pos, const double full_jump_height,
	const double pace, double y_shift, const int y_pitch) {

	const double opp_h = 0.5 + full_jump_height / settings.proj_dist;
	if (y_shift < 0.0) y_shift = 0.0;

	/*
	Uint32* pixbuf_row = (Uint32*) ((Uint8*) screen.pixels + (int) y_shift * screen.pixel_pitch); 
	const Uint32 pixbuf_row_step = screen.pixel_pitch / sizeof(Uint32); // 1000
	*/

	// `y_shift - pace` may go outside the map boundaries; limit this domain
	for (int y = y_shift - pace; y < settings.screen_height - pace; y++) {
	// for (int y = y_shift - pace; y < settings.screen_height - pace; y++, pixbuf_row += pixbuf_row_step) {
		const int row = y - settings.half_screen_height - y_pitch + 1;
		if (row == 0) continue;

		const int pace_y = y + pace;
		const double straight_dist = opp_h / row * settings.proj_dist;

		Uint32* const pixbuf_row = read_texture_row(screen.pixels, screen.pixel_pitch, pace_y);
		/*
		if (cmp_pixbuf_row == pixbuf_row) {
			printf("Equal\n");
		}
		else {
			printf("Not equal\n");
		}
		*/

		for (int screen_x = 0; screen_x < settings.screen_width; screen_x += settings.ray_column_width) {
			const BufferVal buffer_val = val_buffer[screen_x];
			if (buffer_val.wall_bottom >= pace_y + 1) continue;

			const double actual_dist = straight_dist / (double) buffer_val.cos_beta;
			const vec hit = vec_line_pos(pos, buffer_val.dir, actual_dist);

			#ifdef PLANAR_MODE

			if (hit[0] < 1.0 || hit[1] < 1.0 || hit[0] > current_level.map_width - 1.0
				|| hit[1] > current_level.map_height - 1.0) continue;

			#else

			const byte wall_point = map_point(current_level.wall_data, hit[0], hit[1]);
			if (current_level.get_point_height(wall_point, hit)) continue;

			#endif

			draw_from_hit(hit, actual_dist, screen_x, pixbuf_row);
		}
	}
}

#ifdef PLANAR_MODE

void fill_val_buffers_for_planar_mode(const double angle_degrees) {
	const double player_angle = to_radians(angle_degrees);

	for (int screen_x = 0; screen_x < settings.screen_width; screen_x += settings.ray_column_width) {
		const double theta = atan((screen_x - settings.half_screen_width) / settings.proj_dist) + player_angle;
		update_val_buffers(screen_x, 0.0, cos(player_angle - theta), 0.0f, (vec) {cos(theta), sin(theta)});
	}
}

#endif
