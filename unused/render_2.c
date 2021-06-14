const byte get_point_height(const byte point) {
	return 1;
	/*
	switch (point) {
		case 5: return 2;

		case 12: return 1;
		case 13: return 2;
		case 14: return 3;

		default: return 1;
	}
	*/
}

void raycast_2(const Player player) {
	const double player_angle = to_radians(player.angle);

	for (int screen_x = 0; screen_x < screen_width; screen_x += ray_column_width) {
		const double theta =
			atan((screen_x - half_screen_width) / screen.projection_distance) + player_angle;

		const VectorF dir = {cos(theta), sin(theta)};
		long first_wall_hit = 1, lowest_wall_y = screen_height;
		byte curr_point_height = 0;

		// begin DDA
		const VectorF unit_step_size = {fabs(1.0 / dir.x), fabs(1.0 / dir.y)};
		VectorI curr_tile = {floor(player.pos.x), floor(player.pos.y)};
		VectorF ray_step, ray_length;

		if (dir.x < 0)
			ray_step.x = -1.0,
			ray_length.x = (player.pos.x - curr_tile.x) * unit_step_size.x;
		else
			ray_step.x = 1.0,
			ray_length.x = (curr_tile.x + 1.0 - player.pos.x) * unit_step_size.x;

		if (dir.y < 0)
			ray_step.y = -1.0,
			ray_length.y = (player.pos.y - curr_tile.y) * unit_step_size.y;
		else
			ray_step.y = 1.0,
			ray_length.y = (curr_tile.y + 1.0 - player.pos.y) * unit_step_size.y;

		double distance = 0;
		while (1) {
			if (ray_length.x < ray_length.y)
				distance = ray_length.x,
				curr_tile.x += ray_step.x,
				ray_length.x += unit_step_size.x;
			else
				distance = ray_length.y,
				curr_tile.y += ray_step.y,
				ray_length.y += unit_step_size.y;

			if (curr_tile.x < 0 || curr_tile.x >= current_level.map_width ||
				curr_tile.y < 0 || curr_tile.y >= current_level.map_height)
				break;

			const byte point = current_level.wall_data[(int) floor(curr_tile.y)][(int) floor(curr_tile.x)];
			// end DDA
			if (point) {
				const VectorF hit = {dir.x * distance + player.pos.x, dir.y * distance + player.pos.y};
				const double correct_dist = distance * cos(player_angle - theta);

				if (first_wall_hit) {
					screen.z_buffer[screen_x] = correct_dist;
					first_wall_hit = 0;
				}

				const int wall_height = screen.projection_distance / correct_dist;

				const SDL_Rect wall = {
					screen_x,
					half_screen_height - wall_height / 2 + player.y_pitch + player.pace.screen_offset,
					ray_column_width, wall_height
				};

				// overdraw for only different floors - skip if top less than previous top

				const CastData cast_data = {point, distance, hit};
				const byte point_height = get_point_height(point);

				if (point_height > curr_point_height) {
					for (byte i = curr_point_height; i < point_height; i++) {

						SDL_Rect raised_wall = wall;
						raised_wall.y -= wall_height * i;

						if (raised_wall.y + raised_wall.h < 0) break;
						else if (raised_wall.y >= lowest_wall_y) break;
						else lowest_wall_y = raised_wall.y;

						draw_textured_wall(cast_data, dir, raised_wall);
					}
					curr_point_height = point_height;
				}
			}
		}
	}
}
