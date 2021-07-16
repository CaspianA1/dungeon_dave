typedef struct {
	double* const curr_smallest_wall_y;
	const double player_angle, theta, dist, wall_y_shift, full_jump_height;
	const vec begin, hit, dir;
	// const double begin[2], hit[2], dir[2];
	const byte point, side, first_wall_hit;
	const int screen_x;
} DataRaycast;

inlinable int get_wall_tex_offset(const byte side, const vec hit, const vec dir, const int width) {
	const int max_offset = width - 1;

	if (side) {
		const int x_offset = (hit[0] - floor(hit[0])) * max_offset;
		return (dir[1] > 0.0) ? max_offset - x_offset : x_offset;
	}
	else {
		const int y_offset = (hit[1] - floor(hit[1])) * max_offset;
		return (dir[0] < 0.0) ? max_offset - y_offset : y_offset;
	}
}

// float* wall_y_buffer;

vec handle_ray(const DataRaycast d) {
	const double cos_beta = cos(d.player_angle - d.theta);
	const double corrected_dist = d.dist * cos_beta;
	const double wall_h = settings.proj_dist / corrected_dist;
	const byte point_height = current_level.get_point_height(d.point, d.hit);

	const SDL_FRect wall_dest = {
		d.screen_x,
		d.wall_y_shift - wall_h / 2.0 + d.full_jump_height / corrected_dist,
		settings.ray_column_width,
		wall_h
	};

	if (d.first_wall_hit) update_val_buffers(d.screen_x, corrected_dist, cos_beta, wall_dest.y + wall_dest.h, d.dir);

	const Sprite wall_sprite = current_level.walls[d.point - 1];

	//////////
	const SDL_Rect mipmap_crop = get_mipmap_crop(wall_sprite.size, 0);
	const int max_sprite_h = mipmap_crop.h;
	SDL_Rect slice;
	slice.x = get_wall_tex_offset(d.side, d.hit, d.dir, wall_sprite.size.x * 2 / 3);
	slice.y = mipmap_crop.y;
	slice.w = 1;

	if (keys[SDL_SCANCODE_R]) {
		DEBUG_RECT(slice);
		/* example for depth 0: {.x = 74, .y = 0, .w = 1, .h = -2140310760}
		{.x = 99, .y = 0, .w = 1, .h = -2140310760}
		houses = 256x256
		*/
	}

	// const int max_sprite_h = wall_sprite.size.y;
	// SDL_Rect slice = {get_wall_tex_offset(d.side, d.hit, d.dir, wall_sprite.size.x), 0, .w = 1};
	// SDL_Rect slice = {5, 5, 2, 2};
	// DEBUG_RECT(slice);
	//////////

	#ifdef SHADING_ENABLED
	const byte shade = 255 * calculate_shade((double) wall_dest.h, d.hit);
	SDL_SetTextureColorMod(wall_sprite.texture, shade, shade, shade);
	#endif

	/*
	static byte first = 1;
	if (first) {
		wall_y_buffer = calloc(settings.screen_width, sizeof(float));
		first = 0;
	}
	*/

	const double smallest_wall_y = (double) (wall_dest.y - (wall_dest.h * (point_height - 1)));
	// wall_y_buffer[d.screen_x] = smallest_wall_y;

	for (byte i = 0; i < point_height; i++) {
		SDL_FRect raised_wall = wall_dest;
		raised_wall.y -= wall_dest.h * i;

		// completely obscured: starts under the tallest wall so far; wouldn't be seen, but for more speed
		if ((double) raised_wall.y >= *d.curr_smallest_wall_y || raised_wall.y >= settings.screen_height)
			continue;

		// partially obscured: bottom of wall somewhere in middle of tallest
		else if ((double) (raised_wall.y + raised_wall.h) > *d.curr_smallest_wall_y) {
			raised_wall.h = *d.curr_smallest_wall_y - (double) raised_wall.y;
			slice.h = ceil(max_sprite_h * (double) raised_wall.h / wall_h);
		}
		else slice.h = max_sprite_h;

		*d.curr_smallest_wall_y = (double) raised_wall.y;
		if (!keys[SDL_SCANCODE_T]) SDL_RenderCopyF(screen.renderer, wall_sprite.texture, &slice, &raised_wall);
	}
	return (vec) {(double) smallest_wall_y, (double) wall_dest.h};
}

void raycast(const Player player, const double wall_y_shift, const double full_jump_height) {
	const double player_angle = to_radians(player.angle);

	for (int screen_x = 0; screen_x < settings.screen_width; screen_x += settings.ray_column_width) {
		const double theta = atan((screen_x - settings.half_screen_width) / settings.proj_dist) + player_angle;
		const vec dir = {cos(theta), sin(theta)};

		double curr_smallest_wall_y = DBL_MAX, last_height_change_y = settings.screen_height;
		byte at_first_hit = 1, curr_point_height = player.jump.height;
		DataDDA ray = init_dda(player.pos, dir);

		while (iter_dda(&ray)) {
			const byte point = map_point(current_level.wall_data, ray.curr_tile.x, ray.curr_tile.y);
			const vec hit = vec_line_pos(player.pos, dir, ray.dist);
			const byte point_height = current_level.get_point_height(point, hit);

			if (point_height != curr_point_height) {
				double height_change_y, height_change_h;
				if (point) {
					const vec wall_y_components = handle_ray((DataRaycast) {
						&curr_smallest_wall_y, player_angle, theta, ray.dist, wall_y_shift, full_jump_height,
						player.pos, hit, dir, point, ray.side, at_first_hit, screen_x});

					height_change_y = wall_y_components[0], height_change_h = wall_y_components[1];

					at_first_hit = 0;
				}
				else height_change_y = settings.screen_height, height_change_h = 0.0; // correct?

				// draw from last wall y (last height_change_h) to current wall bottom (y + h)
				curr_point_height = point_height;
				last_height_change_y = height_change_y;
			}
		}
	}
}
