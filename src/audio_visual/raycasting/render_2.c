void handle_ray(const Player player, const CastData cast_data, const int screen_x,
	byte* restrict first_wall_hit, double* restrict smallest_wall_y, const double player_angle,
	const double theta, const double wall_y_shift, const double full_jump_height, const VectorF dir) {

	const double cos_beta = cos(player_angle - theta);
	const double correct_dist = cast_data.dist * cos_beta;
	const double wall_h = settings.proj_dist / correct_dist;

	const byte point_height = current_level.get_point_height(cast_data.point, cast_data.hit);

	const SDL_FRect wall = {
		screen_x,
		wall_y_shift - wall_h / 2.0 + full_jump_height / correct_dist,
		settings.ray_column_width,
		wall_h
	};

	if (*first_wall_hit) {
		screen.z_buffer[screen_x] = correct_dist;
		*first_wall_hit = 0;
	}

	for (byte i = 0; i < point_height; i++) {
		SDL_FRect raised_wall = wall;
		raised_wall.y -= wall.h * i;

		// completely obscured: starts under the tallest so far
		if ((double) raised_wall.y >= *smallest_wall_y) continue;

		const int max_sprite_height =
			current_level.walls[cast_data.point - 1].surface -> h - 1;

		const double
			raised_wall_bottom = (double) (raised_wall.y + raised_wall.h),
			max_raised_wall_h = (double) raised_wall.h;

		double sprite_height;

		// fully visible: bottom smaller than smallest top
		if (raised_wall_bottom <= *smallest_wall_y) {
			sprite_height = max_sprite_height;
			if ((double) raised_wall.y < *smallest_wall_y)
				*smallest_wall_y = (double) raised_wall.y;
			if (i == 0) std_draw_floor(player, dir, raised_wall, cos_beta);
		}

		else { // partially obscured: bottom of wall somewhere in middle of tallest
			const double y_obscured = raised_wall_bottom - *smallest_wall_y;
			const double init_raised_h = (double) raised_wall.h;

			raised_wall.h -= (float) y_obscured;
			if (doubles_eq((double) raised_wall.h, 0.0, std_double_epsilon)) continue;

			sprite_height = max_sprite_height * (double) raised_wall.h / init_raised_h;

			if ((double) raised_wall.y < *smallest_wall_y)
				*smallest_wall_y = (double) raised_wall.y;
		}
		draw_wall(cast_data, dir, raised_wall, sprite_height, max_raised_wall_h);
	}
}

void raycast_2(const Player player, const double wall_y_shift, const double full_jump_height) {
	const double player_angle = to_radians(player.angle);

	for (int screen_x = 0; screen_x < settings.screen_width; screen_x += settings.ray_column_width) {
		const double theta = atan((screen_x - settings.half_screen_width)
			/ settings.proj_dist) + player_angle;

		const VectorF dir = {cos(theta), sin(theta)};

		// begin DDA
		byte first_wall_hit = 1, side;
		double smallest_wall_y = DBL_MAX;

		const VectorF unit_step_size = {fabs(1.0 / dir[0]), fabs(1.0 / dir[1])};
		VectorF ray_length;
		VectorI curr_tile = VectorF_floor(player.pos), ray_step;

		if (dir[0] < 0.0)
			ray_step.x = -1,
			ray_length[0] = (player.pos[0] - curr_tile.x) * unit_step_size[0];
		else
			ray_step.x = 1,
			ray_length[0] = (curr_tile.x + 1.0 - player.pos[0]) * unit_step_size[0];

		if (dir[1] < 0.0)
			ray_step.y = -1,
			ray_length[1] = (player.pos[1] - curr_tile.y) * unit_step_size[1];
		else
			ray_step.y = 1,
			ray_length[1] = (curr_tile.y + 1.0 - player.pos[1]) * unit_step_size[1];

		double distance = 0;
		while (1) {
			if (ray_length[0] < ray_length[1])
				distance = ray_length[0],
				curr_tile.x += ray_step.x,
				ray_length[0] += unit_step_size[0],
				side = 0;
			else
				distance = ray_length[1],
				curr_tile.y += ray_step.y,
				ray_length[1] += unit_step_size[1],
				side = 1;

			if (curr_tile.x < 0 || curr_tile.x >= current_level.map_width ||
				curr_tile.y < 0 || curr_tile.y >= current_level.map_height)
				break;

			const byte point = wall_point(curr_tile.x, curr_tile.y);
			if (point) {
				const CastData cast_data = {
					point, side, distance, VectorF_line_pos(player.pos, dir, distance)
				};

				handle_ray(player, cast_data, screen_x,
					&first_wall_hit, &smallest_wall_y,
					player_angle, theta, wall_y_shift, full_jump_height, dir);
			}
		}
		// end DDA
	}
}
