static Uint32 temp_buf[INIT_H][INIT_W]; // max: 1440 by 900

// Why doesn't this work on more FOVs?
void draw_floor_plane(const Player player) {
	// store pixels in a local buffer, and write them afterwards? vram is far away
	// https://stackoverflow.com/questions/2963898/faster-alternative-to-memcpy

	const int
		begin_x = 0, end_x = settings.screen_width,
		begin_y = 0, end_y = settings.half_screen_height;

	const byte is_floor = 1;

	const double
		theta = to_radians(player.angle),
		screen_height_over_proj_dist = settings.screen_height / settings.proj_dist,
		y_shift = player.pace.screen_offset + player.z_pitch; // in screen space

	const double
		abs_y_shift = fabs(y_shift),
		screen_z = settings.half_screen_height +
			player.jump.height * settings.screen_height * screen_height_over_proj_dist;

	const map_data point_data = is_floor ? current_level.floor_data : current_level.ceiling_data;

	const VectorF dir = {
		cos(theta) / screen_height_over_proj_dist,
		sin(theta) / screen_height_over_proj_dist
	};

	const VectorF
		/* for 90 degrees, this works, b/c the camera plane and direction vector are parallel.
		visualize the vectors and see how non-90 degree angles need to be adjusted. */
		plane = {-dir[1], dir[0]},
		screen_width_vec = VectorF_memset(settings.screen_width);

	const VectorF
		ray_dir_begin = VectorFF_sub(dir, plane),
		ray_dir_diff = VectorFF_mul(plane, VectorF_memset(2.0));

	for (int y = begin_y - abs_y_shift; y < end_y + abs_y_shift - 1; y++) {
		if (y == settings.half_screen_height) break;

		const int screen_y = (is_floor ? settings.screen_height - y : y) + y_shift;
		if (screen_y < 0 || screen_y >= settings.screen_height)
			continue;

		const int row = abs(y - settings.half_screen_height);

		const VectorF row_dist = VectorF_memset(screen_z / row);
		const VectorF step = VectorFF_div(VectorFF_mul(row_dist, ray_dir_diff), screen_width_vec);
		VectorF ray_pos = VectorFF_add(VectorFF_mul(ray_dir_begin, row_dist), player.pos);

		for (int x = begin_x; x < end_x; x++) {
			const VectorF cell = get_cell_from_ray_pos(ray_pos);
			const byte point = point_data[(int) cell[1]][(int) cell[0]];
			const SDL_Surface* restrict surface = current_level.walls[point - 1].surface;

			const VectorF tex_w = VectorF_memset(surface -> w);
			const int max_offset = tex_w[0] - 1;
			const VectorF offset = VectorFF_mul(tex_w, VectorFF_sub(ray_pos, cell));

			/////
			const Uint32 src = get_surface_pixel(
				surface -> pixels, surface -> pitch,
				(int) offset[0] & max_offset,
				(int) offset[1] & max_offset);

			temp_buf[screen_y][x] = src; // also aligned now (?)
			// *get_pixbuf_pixel(x, screen_y) = src; // aligned
			ray_pos = VectorFF_add(ray_pos, step);
		}
	}
}

inlinable void draw_ceiling_plane(const Player player) {
	const int
		begin_x = 0, end_x = settings.screen_width,
		begin_y = 0, end_y = settings.half_screen_height;

	const byte is_floor = 0;

	const double
		theta = to_radians(player.angle),
		screen_height_over_proj_dist = settings.screen_height / settings.proj_dist,
		y_shift = player.pace.screen_offset + player.z_pitch; // in screen space

	//////////
	const double
		abs_y_shift = fabs(y_shift), // I need another constant than 2.0
		reverse_p_height = current_level.max_point_height - player.jump.height - 2.0;

	const double screen_z = settings.half_screen_height +
		reverse_p_height * settings.screen_height / screen_height_over_proj_dist;
	//////////

	const map_data point_data = is_floor ? current_level.floor_data : current_level.ceiling_data;

	const VectorF dir = {
		cos(theta) * screen_height_over_proj_dist,
		sin(theta) * screen_height_over_proj_dist
	};

	const VectorF
		/* for 90 degrees, this works, b/c the camera plane and direction vector are parallel.
		visualize the vectors and see how non-90 degree angles need to be adjusted. */
		plane = {-dir[1], dir[0]},
		screen_width_vec = VectorF_memset(settings.screen_width);

	const VectorF
		ray_dir_begin = VectorFF_sub(dir, plane),
		ray_dir_diff = VectorFF_mul(plane, VectorF_memset(2.0));

	for (int y = begin_y - abs_y_shift; y < end_y + abs_y_shift - 1; y++) {
		if (y == settings.half_screen_height) break;

		const int screen_y = (is_floor ? settings.screen_height - y : y) + y_shift;
		if (screen_y < 0 || screen_y >= settings.screen_height)
			continue;

		const int row = abs(y - settings.half_screen_height);

		const VectorF row_dist = VectorF_memset(screen_z / row);
		const VectorF step = VectorFF_div(VectorFF_mul(row_dist, ray_dir_diff), screen_width_vec);
		VectorF ray_pos = VectorFF_add(VectorFF_mul(ray_dir_begin, row_dist), player.pos);

		for (int x = begin_x; x < end_x; x++) {
			const VectorF cell = get_cell_from_ray_pos(ray_pos);
			const byte point = point_data[(int) cell[1]][(int) cell[0]];
			const SDL_Surface* restrict surface = current_level.walls[point - 1].surface;

			const VectorF tex_w = VectorF_memset(surface -> w);
			const int max_offset = tex_w[0] - 1;
			const VectorF offset = VectorFF_mul(tex_w, VectorFF_sub(ray_pos, cell));

			/////
			const Uint32 src = get_surface_pixel(
				surface -> pixels, surface -> pitch,
				(int) offset[0] & max_offset,
				(int) offset[1] & max_offset);

			temp_buf[screen_y][x] = src; // also aligned now (?)
			// *get_pixbuf_pixel(x, screen_y) = src; // aligned
			ray_pos = VectorFF_add(ray_pos, step);
		}
	}
}

inlinable void refresh_and_clear_temp_buf(void) {
	const size_t buf_size = settings.screen_height * settings.screen_width * sizeof(Uint32);
	memcpy(screen.pixels, temp_buf, buf_size);
	memset(temp_buf, 0, buf_size);
}
