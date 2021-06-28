typedef struct {
	const byte point, side;
	const double dist;
	const VectorF hit;
} CastData;

inlinable int calculate_wall_tex_offset(const CastData cast_data, const VectorF dir, const int width) {
	const int max_offset = width - 1;

	if (cast_data.side) {
		const int x_offset = (cast_data.hit[0] - floor(cast_data.hit[0])) * max_offset;
		return (dir[1] > 0.0) ? max_offset - x_offset : x_offset;
	}
	else {
		const int y_offset = (cast_data.hit[1] - floor(cast_data.hit[1])) * max_offset;
		return (dir[0] < 0.0) ? max_offset - y_offset : y_offset;
	}
}

void handle_ray(const Player player, const CastData cast_data, const int screen_x,
	byte* const first_wall_hit, double* const smallest_wall_y, const double player_angle,
	const double theta, const double wall_y_shift, const double full_jump_height, const VectorF dir) {

	#ifdef PLANAR_MODE
	(void) player;
	#endif

	const double cos_beta = cos(player_angle - theta);
	const double corrected_dist = cast_data.dist * cos_beta;
	const double wall_h = settings.proj_dist / corrected_dist;

	const byte point_height = current_level.get_point_height(cast_data.point, cast_data.hit);

	const SDL_FRect wall = {
		screen_x,
		wall_y_shift - wall_h / 2.0 + full_jump_height / corrected_dist,
		settings.ray_column_width,
		wall_h
	};

	if (*first_wall_hit) {
		update_z_buffer(screen_x, corrected_dist);
		*first_wall_hit = 0;
	}

	// const int max_sprite_h = current_level.walls[cast_data.point - 1].surface -> h;

	int offset;
	Sprite wall_sprite;

	for (byte i = 0, first_draw_event = 1; i < point_height; i++) {
		SDL_FRect raised_wall = wall;
		raised_wall.y -= wall.h * i;

		// completely obscured: starts under the tallest so far
		if ((double) raised_wall.y >= *smallest_wall_y) continue;

		const double raised_wall_bottom = (double) (raised_wall.y + raised_wall.h);

		if (raised_wall_bottom > *smallest_wall_y) { // partially obscured: bottom of wall somewhere in middle of tallest
			const double y_obscured = raised_wall_bottom - *smallest_wall_y;
			raised_wall.h -= (float) y_obscured;
			if (doubles_eq((double) raised_wall.h, 0.0, std_double_epsilon)) continue;
		}

		#ifndef PLANAR_MODE

		else if (i == 0) std_draw_floor(player, dir, raised_wall, cos_beta);

		#endif

		if ((double) raised_wall.y < *smallest_wall_y) *smallest_wall_y = (double) raised_wall.y;

		if (first_draw_event) {
			wall_sprite = current_level.walls[cast_data.point - 1];	
			offset = calculate_wall_tex_offset(cast_data, dir, wall_sprite.surface -> w);
			const byte shade = 255 * calculate_shade((double) wall.h, cast_data.hit);
			SDL_SetTextureColorMod(wall_sprite.texture, shade, shade, shade);
			first_draw_event = 0;

			#ifndef PLANAR_MODE
			if (i == 0) std_draw_floor(player, dir, raised_wall, cos_beta);
			#endif
		}

		const SDL_Rect slice = {offset, 0, 1, wall_sprite.surface -> h};
		SDL_RenderCopyF(screen.renderer, wall_sprite.texture, &slice, &raised_wall);
	}
}

inlinable void dda_step(double* const distance, VectorI* const curr_tile_ref,
	VectorF* const ray_length_ref, byte* const side,
	const VectorF unit_step_size, const VectorI ray_step) {

	VectorI curr_tile = *curr_tile_ref;
	VectorF ray_length = *ray_length_ref;

	if (ray_length[0] < ray_length[1]) {
		*distance = ray_length[0];
		curr_tile.x += ray_step.x;
		ray_length[0] += unit_step_size[0];
		*side = 0;
	}
	else {
		*distance = ray_length[1];
		curr_tile.y += ray_step.y;
		ray_length[1] += unit_step_size[1];
		*side = 1;
	}

	*curr_tile_ref = curr_tile;
	*ray_length_ref = ray_length;
}

void raycast(const Player player, const double wall_y_shift, const double full_jump_height) {
	const double player_angle = to_radians(player.angle);

	for (int screen_x = 0; screen_x < settings.screen_width; screen_x += settings.ray_column_width) {
		const double theta = atan((screen_x - settings.half_screen_width) / settings.proj_dist) + player_angle;

		const VectorF dir = {cos(theta), sin(theta)};

		// begin DDA
		byte first_wall_hit = 1, side;
		double smallest_wall_y = DBL_MAX;

		const VectorF unit_step_size = {fabs(1.0 / dir[0]), fabs(1.0 / dir[1])};
		VectorF ray_length, origin = player.pos;
		VectorI curr_tile = VectorF_floor(origin), ray_step;

		if (dir[0] < 0.0) {
			ray_step.x = -1;
			ray_length[0] = (origin[0] - curr_tile.x) * unit_step_size[0];
		}
		else {
			ray_step.x = 1;
			ray_length[0] = (curr_tile.x + 1.0 - origin[0]) * unit_step_size[0];
		}

		if (dir[1] < 0.0) {
			ray_step.y = -1;
			ray_length[1] = (origin[1] - curr_tile.y) * unit_step_size[1];
		}
		else {
			ray_step.y = 1;
			ray_length[1] = (curr_tile.y + 1.0 - origin[1]) * unit_step_size[1];
		}

		double distance = 0;
		while (1) {
			dda_step(&distance, &curr_tile, &ray_length, &side, unit_step_size, ray_step);

			if (curr_tile.x < 0 || curr_tile.x >= current_level.map_width ||
				curr_tile.y < 0 || curr_tile.y >= current_level.map_height)
				break;

			const byte point = map_point(current_level.wall_data, curr_tile.x, curr_tile.y);
			if (point) {
				const CastData cast_data = {point, side, distance, VectorF_line_pos(player.pos, dir, distance)};
				handle_ray(player, cast_data, screen_x, &first_wall_hit, &smallest_wall_y,
					player_angle, theta, wall_y_shift, full_jump_height, dir);
			}
		}
		// end DDA
	}
}
