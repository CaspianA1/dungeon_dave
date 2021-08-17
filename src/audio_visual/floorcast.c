inlinable vec vec_tex_offset(const vec pos, const int tex_size) {
	return vec_fill(tex_size) * (pos - vec_trunc(pos));
}

inlinable Uint32* read_texture_row(const void* const pixels, const int pixel_pitch, const int y) {
	return (Uint32*) ((Uint8*) pixels + y * pixel_pitch);
}

#ifdef SHADING_ENABLED

inlinable Uint32 shade_ARGB_pixel(const Uint32 pixel, const double shade) {
	const byte r = (byte) (pixel >> 16) * shade, g = (byte) (pixel >> 8) * shade, b = (byte) pixel * shade;
	return 0xFF000000 | (r << 16) | (g << 8) | b;
}

#endif

PSprite p;
inlinable void draw_from_hit(const vec hit, const double dist, const int screen_x, Uint32* const pixbuf_row) {
	/* for mesa.bmp
	static byte print_bad = 1;
	const Uint32 top = read_texture_row(p.pixels, p.pitch, 0)[0];
	if (top != 4293837722 && print_bad) {
		printf("Schlecht; reads = %llu\n", reads);
		// 8427098
		print_bad = 0;
	}
	*/

	const vec offset = vec_tex_offset(hit, p.size);
	Uint32 pixel = read_texture_row(p.pixels, p.pitch, offset[1])[(long) offset[0]];

	#ifdef SHADING_ENABLED
	pixel = shade_ARGB_pixel(pixel, calculate_shade(settings.proj_dist / dist, hit));
	#else
	(void) dist;
	#endif

	for (byte x = 0; x < settings.ray_column_width; x++) pixbuf_row[x + screen_x] = pixel;
}

vec lerp_floor(const vec pos, const double straight_dist, vec* const hit) {
	const BufferVal
		start_buf_val = val_buffer[0],
		end_buf_val = val_buffer[settings.screen_width - 1];

	const double
		start_actual_dist = straight_dist / (double) start_buf_val.cos_beta,
		end_actual_dist = straight_dist / (double) end_buf_val.cos_beta;

	const vec
		start_hit = vec_line_pos(pos, start_buf_val.dir, start_actual_dist),
		end_hit = vec_line_pos(pos, end_buf_val.dir, end_actual_dist);

	/*
	DEBUG_VEC(start_hit);
	DEBUG_VEC(end_hit);
	*/

	const vec step = (end_hit - start_hit) / vec_fill(settings.screen_width);
	*hit = start_hit;

	return step;
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

		// vec hit;
		// const vec step = lerp_floor(pos, straight_dist, &hit);
		for (int screen_x = 0; screen_x < settings.screen_width; screen_x++) {
			if (get_statemap_bit(occluded_by_walls, screen_x, pace_y)) continue;

			const BufferVal buffer_val = val_buffer[screen_x];

			const double actual_dist = straight_dist / (double) buffer_val.cos_beta;
			const vec hit = vec_line_pos(pos, buffer_val.dir, actual_dist);

			if (hit[0] < 1.0 || hit[1] < 1.0 || hit[0] > current_level.map_size.x - 1.0
				|| hit[1] > current_level.map_size.y - 1.0) continue;

			#ifndef PLANAR_MODE
			const byte wall_point = map_point(current_level.wall_data, hit[0], hit[1]);
			if (current_level.get_point_height(wall_point, hit) != floor_height) continue;
			#endif

			draw_from_hit(hit, actual_dist, screen_x, pixbuf_row);
			// hit += step;
		}
	}
}

#ifdef PLANAR_MODE

void fill_val_buffers_for_planar_mode(const double angle_degrees) {
	const double player_angle = to_radians(angle_degrees);

	for (int screen_x = 0; screen_x < settings.screen_width; screen_x += settings.ray_column_width) {
		const double theta = atan((screen_x - settings.half_screen_width) / settings.proj_dist) + player_angle;
		update_val_buffers(screen_x, 0, 0, 0.0f, cosf(player_angle - theta), (vec) {cos(theta), sin(theta)});
	}
}

#endif

void draw_colored_floor(const double y_shift) {
	const SDL_FRect floor = {0.0, y_shift - 1.0, settings.screen_width, settings.screen_height - y_shift + 1.0};
	SDL_SetRenderDrawColor(screen.renderer, 148, 107, 69, SDL_ALPHA_OPAQUE);
	SDL_RenderFillRectF(screen.renderer, &floor);
}
