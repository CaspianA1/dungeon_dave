inlinable vec vec_tex_offset(const vec pos, const int tex_size) {
	return vec_fill(tex_size) * (pos - vec_trunc(pos));
}

inlinable Uint32* read_texture_row(const void* const pixels, const int pixel_pitch, const int y) {
	return (Uint32*) ((Uint8*) pixels + y * pixel_pitch);
}

#ifdef SHADING_ENABLED

inlinable Uint32 shade_ARGB_pixel(const Uint32 pixel, const byte shade) {
	int r = (byte) (pixel >> 16), g = (byte) (pixel >> 8), b = (byte) pixel;

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
void fast_affine_floor(const byte floor_height, const vec pos, const double p_height, const int y_shift) {
	const double screen_height_proj_ratio = settings.screen_height / settings.proj_dist;
	const double world_height = p_height - floor_height / screen_height_proj_ratio;

	if (world_height < settings.plane_bottom) return; // if the player is under the floor plane
	const double opp_h = 0.5 + world_height * screen_height_proj_ratio;

	for (int row = 1; row <= settings.screen_height - y_shift; row++) {
		const double straight_dist = opp_h / row * settings.proj_dist;
		const int pace_y = y_shift + row - 1;

		if (pace_y < 0) continue;

		Uint32* const pixbuf_row = read_texture_row(screen.pixels, screen.pixel_pitch, pace_y);

		for (int screen_x = 0; screen_x < settings.screen_width; screen_x += settings.ray_column_width) {
			/* The remaining bottlenecks:
			Checking for statemap bits will not be needed with a visplane system -> speedup
			Also, that means that the statemap won't be needed anymore -> speedup
			Checking for out-of-bound hits won't be needed either -> speedup
			Checking for wall points that do not equal the floor height won't be needed either -> speedup
			Also, with a visplane system, less y and x coordinates will be iterated over -> speedup */

			if (get_statemap_bit(occluded_by_walls, screen_x, pace_y)) continue;

			const BufferVal buffer_val = val_buffer[screen_x];
			const double actual_dist = straight_dist * (double) buffer_val.one_over_cos_beta;
			const vec hit = vec_line_pos(pos, buffer_val.dir, actual_dist);

			if (hit[0] < 1.0 || hit[1] < 1.0 || hit[0] > current_level.map_size.x - 1.0
				|| hit[1] > current_level.map_size.y - 1.0) continue;

			#ifndef PLANAR_MODE
			const byte wall_point = map_point(current_level.wall_data, hit[0], hit[1]);
			if (current_level.get_point_height(wall_point, hit) != floor_height) continue;
			#endif

			const vec offset = vec_tex_offset(hit, ground.size);
			Uint32 pixel = ground.pixels[(long) ((long) offset[1] * ground.size + offset[0])];

			#ifdef SHADING_ENABLED
			pixel = shade_ARGB_pixel(pixel, calculate_shade(settings.proj_dist / actual_dist, hit));
			#endif

			for (int x = 0; x < settings.ray_column_width; x++) pixbuf_row[x + screen_x] = pixel;
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
