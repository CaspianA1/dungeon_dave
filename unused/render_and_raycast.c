void draw_2D_map() {
	SDL_SetRenderDrawColor(screen.renderer_2D, 210, 180, 140, SDL_ALPHA_OPAQUE);

	SDL_Rect wall = {0, 0, round(width_ratio), round(height_ratio)};

	for (int map_x = 0; map_x < screen_width; map_x++) {
		for (int map_y = 0; map_y < screen_height; map_y++) {
			if (is_a_wall(map[map_y][map_x])) {
				wall.x = map_x * width_ratio, wall.y = map_y * height_ratio;
				SDL_RenderFillRect(screen.renderer_2D, &wall);
			}
		}
	}
}

void draw_untextured_wall(const int wall_height, const byte point, const SDL_Rect pace_wall_pos) {
	byte r, g, b;
	switch (point) {
		case 1: r = 255, g = 255, b = 0; break;
		case 2: r = 0, g = 128, b = 128; break;
		case 3: r = 255, g = 165, b = 0; break;
		case 4: r = 255, g = 0, b = 0; break;
	}
	const double shade = calculate_shade(wall_height);
	draw_rectangle(&pace_wall_pos, r * shade, g * shade, b * shade);
}

void draw_textured_wall(const int wall_height, const byte point,
	const SDL_Rect pace_wall_pos, const double new_x, const double new_y) {

	const Sprite wall_sprite = textured_walls[point - (wall_count - textured_wall_count + 1)];

	const int max_offset = wall_sprite.surface -> w - 1;

	const double
		diff_x = new_x - floor(new_x),
		diff_y = new_y - floor(new_y);

	const int on_x = close_to_whole(diff_y), on_y = close_to_whole(diff_x);

	const int offset =
		(on_x ^ on_y)
			? (int) ((on_x ? diff_x : diff_y) * (max_offset + 1)) & max_offset
			: max_offset;

	render_scanline(wall_height, wall_sprite, offset, &pace_wall_pos);
}

void draw_floor(const SDL_Rect wall_pos, const Player player,
	const double cos_theta, const double sin_theta, const double cos_beta) {

	const Sprite sprite = textured_walls[0];
	const SDL_Surface* surface = sprite.surface;
	const SDL_PixelFormat* format = surface -> format;
	const void* pixels = surface -> pixels;

	const int
		surface_pitch = surface -> pitch,
		sprite_w = sprite.surface -> w;

	for (int y = wall_pos.y + wall_pos.h; y < screen_height - player.pace.screen_offset; y++) {
		const int floor_y = y + player.pace.screen_offset;
		if (floor_y < 0) continue;

		const int floor_row = y - half_screen_height - player.y_pitch.val;

		const double floor_straight_distance = 0.5 / floor_row * screen.projection_distance;
		const double floor_actual_distance = floor_straight_distance / cos_beta;

		const double 
			hit_x = cos_theta * floor_actual_distance + player.pos.x,
			hit_y = sin_theta * floor_actual_distance + player.pos.y;

		const int
			tex_x = (int) ((hit_x - floor(hit_x)) * sprite_w) & sprite_w - 1,
			tex_y = (int) ((hit_y - floor(hit_y)) * sprite_w) & sprite_w - 1;

		Uint32 surface_pixel = get_surface_pixel(pixels, surface_pitch, tex_x, tex_y);
		SDL_Color pixel;
		SDL_GetRGB(surface_pixel, format, &pixel.r, &pixel.g, &pixel.b);

		// there's no wall here (of course), but this is used for the shading calculation
		const double shade = calculate_shade(screen.projection_distance / floor_actual_distance);
		surface_pixel = SDL_MapRGB(format, pixel.r * shade, pixel.g * shade, pixel.b * shade);
		*get_pixbuf_pixel(wall_pos.x, floor_y) = surface_pixel;
	}
}

void raycast(const Player player) {
	const double rad_player_angle = to_radians(player.angle);
	int screen_x = 0;

	for (double theta = player.angle - half_fov; theta < player.angle + half_fov; theta += theta_step) {
		const double rad_theta = to_radians(theta);
		const double cos_theta = cos(rad_theta), sin_theta = sin(rad_theta);
		double dist = 0;

		while (1) {
			const double
				new_x = cos_theta * dist + player.pos.x,
				new_y = sin_theta * dist + player.pos.y;

			const byte point = map[(int) new_y][(int) new_x];

			if (is_a_wall(point)) {

				#ifdef MODE_2D
				SDL_RenderDrawLine(screen.renderer_2D,
					player.pos.x * width_ratio, player.pos.y * height_ratio,
					new_x * width_ratio, new_y * height_ratio);
				#endif

				const double beta = rad_player_angle - rad_theta;
				const double cos_beta = cos(beta);

				dist *= cos_beta;
				screen.z_buffer[screen_x] = dist;

				const int wall_height = screen.projection_distance / dist;

				const SDL_Rect wall_pos = {
					screen_x,
					half_screen_height - wall_height / 2 + player.y_pitch.val,
					1, wall_height
				};

				SDL_Rect pace_wall = wall_pos; pace_wall.y += player.pace.screen_offset;

				if (point <= plain_wall_count)
					draw_untextured_wall(wall_height, point, pace_wall);
				else
					draw_textured_wall(wall_height, point, pace_wall, new_x, new_y);

				draw_floor(wall_pos, player, cos_theta, sin_theta, cos_beta);

				screen_x++;
				break;
			}
			dist += dist_step;
		}
	}
	draw_sprites(player);
}
