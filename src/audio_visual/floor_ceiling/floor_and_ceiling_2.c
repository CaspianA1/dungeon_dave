void simd_draw_floor_or_ceil(const VectorF pos, const VectorF dir, 
	const byte is_ceiling, const int screen_x, const double begin, const double end,
	const double cos_beta, double pace, const int z_pitch, const double p_height) {

	if (end <= begin) return;

	map_data tex_hit_data;
	int (*get_row) (const double, const int);

	if (is_ceiling) {
		tex_hit_data = current_level.ceiling_data;
		get_row = get_ceil_row;
	}

	else {
		tex_hit_data = current_level.floor_data;
		get_row = get_floor_row;
	}

	const double p_height_ratio = p_height * settings.screen_height / settings.proj_dist;
	const double opp_h = 0.5 + (is_ceiling ? -p_height_ratio : p_height_ratio);

	const VectorF
		opp_h_vec = VectorF_memset(opp_h),
		proj_dist_vec = VectorF_memset(settings.proj_dist),
		cos_beta_vec = VectorF_memset(cos_beta);

	double
		hit_w_limit = current_level.map_width,
		hit_h_limit = current_level.map_height;

	for (int y = begin - pace; y < (int) end - pace; y += 2) {
		const int next_y = y + 1;

		const VectorF rows = {get_row(y, z_pitch), get_row(next_y, z_pitch)};
		const VectorF straight_dists = VectorFF_mul(VectorFF_div(opp_h_vec, rows), proj_dist_vec);
		const VectorF actual_dists = VectorFF_div(straight_dists, cos_beta_vec);

		const VectorF2 hits = VectorF2_line_pos(pos, dir, actual_dists);
		// {hit.x1, hit.y1, hit.x1, hit.y1}

		if (hits[0] < 0.0 || hits[1] < 0.0 || hits[2] < 0.0 || hits[3] < 0.0
			|| hits[0] > hit_w_limit || hits[1] > hit_h_limit
			|| hits[2] > hit_w_limit || hits[3] > hit_h_limit)
			continue;

		const VectorI floored_hits[2] = {
			{(int) floor(hits[0]), (int) floor(hits[1])},
			{(int) floor(hits[2]), (int) floor(hits[3])}
		};

		const byte points[2] = {
			tex_hit_data[floored_hits[0].y][floored_hits[0].x],
			tex_hit_data[floored_hits[1].y][floored_hits[1].x]
		};

		const SDL_Surface* restrict surfaces[2] = {
			current_level.walls[points[0] - 1].surface,
			current_level.walls[points[1] - 1].surface
		};

		const int max_offsets[2] = {surfaces[0] -> w - 1, surfaces[1] -> w - 1};

		const VectorI surface_offsets[2] = {
			{(hits[0] - floored_hits[0].x) * max_offsets[0],
			(hits[1] - floored_hits[0].y) * max_offsets[0]},

			{(hits[2] - floored_hits[1].x) * max_offsets[1],
			(hits[3] - floored_hits[1].y) * max_offsets[1]}
		};

		// bottleneck
		Uint32 surface_pixels[2] = {
			get_surface_pixel(surfaces[0] -> pixels, surfaces[0] -> pitch,
				surface_offsets[0].x, surface_offsets[0].y),

			get_surface_pixel(surfaces[1] -> pixels, surfaces[1] -> pitch,
				surface_offsets[1].x, surface_offsets[1].y)
		};

		/////
		const VectorF first_hit = {hits[0], hits[1]}, second_hit = {hits[2], hits[3]};

		#ifndef SHADING_ENABLED
		(void) first_hit;
		(void) second_hit;
		#endif

		#ifdef FULL_QUALITY
		int dest_y = y + pace;
		if (dest_y >= settings.screen_height)
			dest_y = settings.screen_height - 1;
		else if (dest_y < 0)
			dest_y = 0;

		*get_pixbuf_pixel(screen_x, dest_y) =
			shade_ARGB_pixel(surface_pixels[0], actual_dists[0], first_hit);

		int next_dest_y = next_y + pace;
		if (next_dest_y >= settings.screen_height)
			next_dest_y = settings.screen_height - 1;
		else if (next_dest_y < 0)
			next_dest_y = 0;

		*get_pixbuf_pixel(screen_x, next_dest_y) =
			shade_ARGB_pixel(surface_pixels[1], actual_dists[1], second_hit);
		/////
		#else
		/////
		int pace_y_vals[2] = {y + pace, next_y + pace};
		if (pace_y_vals[1] >= settings.screen_height)
			pace_y_vals[1] = settings.screen_height - 1;

		surface_pixels[0] = shade_ARGB_pixel(surface_pixels[0], actual_dists[0], first_hit);
		surface_pixels[1] = shade_ARGB_pixel(surface_pixels[1], actual_dists[1], second_hit);

		Uint32
			*dest_1 = get_pixbuf_pixel(screen_x, pace_y_vals[0]),
			*dest_2 = get_pixbuf_pixel(screen_x, pace_y_vals[1]);

		for (int offset_x = 0; offset_x < settings.ray_column_width; offset_x++) {
			*(dest_1 + offset_x) = surface_pixels[0];
			*(dest_2 + offset_x) = surface_pixels[1];
		}
		/////
		#endif
	}
}
