typedef struct {
	const byte point, side;
	const double dist;
	const VectorF hit;
} CastData;

/*
extended cast data:
*/

inlinable int calculate_wall_tex_offset(const byte side, const VectorF hit, const VectorF dir, const int width) {
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

void old_handle_ray(const Player player, const CastData cast_data, const int screen_x,
	const byte first_wall_hit, double* const smallest_wall_y, const double player_angle,
	const double theta, const double wall_y_shift, const double full_jump_height, const VectorF dir) {

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

	if (first_wall_hit) update_z_buffer(screen_x, corrected_dist);

	const int max_sprite_h = current_level.walls[cast_data.point - 1].surface -> h;

	int offset;
	Sprite wall_sprite;

	for (byte i = 0, first_draw_event = 1; i < point_height; i++) {
		SDL_FRect raised_wall = wall;
		raised_wall.y -= wall.h * i;

		// completely obscured: starts under the tallest so far
		if ((double) raised_wall.y >= *smallest_wall_y) continue;

		const double raised_wall_bottom = (double) (raised_wall.y + raised_wall.h);
		int sprite_height;

		// fully visible: bottom smaller than smallest top
		if (raised_wall_bottom <= *smallest_wall_y) {
			sprite_height = max_sprite_h;

			#ifndef PLANAR_MODE
			if (i == 0) std_draw_floor(player, dir, raised_wall, cos_beta);
			#endif
		}

		else { // partially obscured: bottom of wall somewhere in middle of tallest
			const double
				y_obscured = raised_wall_bottom - *smallest_wall_y,
				init_raised_h = (double) raised_wall.h;

			raised_wall.h -= (float) y_obscured;
			if (doubles_eq((double) raised_wall.h, 0.0, std_double_epsilon)) continue;

			sprite_height = max_sprite_h;

			// if (player.jump.height >= i)
			sprite_height *= (double) raised_wall.h / init_raised_h;
		}

		if ((double) raised_wall.y < *smallest_wall_y) *smallest_wall_y = (double) raised_wall.y;

		if (first_draw_event) {
			wall_sprite = current_level.walls[cast_data.point - 1];	
			offset = calculate_wall_tex_offset(cast_data.side, cast_data.hit, dir, wall_sprite.surface -> w);
			const byte shade = 255 * calculate_shade((double) wall.h, cast_data.hit);
			SDL_SetTextureColorMod(wall_sprite.texture, shade, shade, shade);
			first_draw_event = 0;
		}

		const SDL_Rect slice = {offset, 0, 1, sprite_height};
		SDL_RenderCopyF(screen.renderer, wall_sprite.texture, &slice, &raised_wall);
	}
}

void handle_ray(const Player player, const CastData cast_data, const int screen_x,
	const byte first_wall_hit, double* const smallest_wall_y, const double player_angle,
	const double theta, const double wall_y_shift, const double full_jump_height, const VectorF dir) {

	// this one is slower

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

	if (first_wall_hit) update_z_buffer(screen_x, corrected_dist);

	int offset;
	Sprite wall_sprite;

	for (byte i = 0, first_draw_event = 1; i < point_height; i++) {
		SDL_FRect raised_wall = wall;
		raised_wall.y -= wall.h * i;

		// completely obscured: starts under the tallest so far
		if ((double) raised_wall.y >= *smallest_wall_y) continue;

		const double raised_wall_bottom = (double) (raised_wall.y + raised_wall.h);

		// partially obscured: bottom of wall somewhere in middle of tallest
		if (raised_wall_bottom > *smallest_wall_y) {
			const double y_obscured = raised_wall_bottom - *smallest_wall_y;
			raised_wall.h -= (float) y_obscured;
			if (doubles_eq((double) raised_wall.h, 0.0, std_double_epsilon)) continue;
		}

		if ((double) raised_wall.y < *smallest_wall_y) *smallest_wall_y = (double) raised_wall.y;

		if (first_draw_event) {
			wall_sprite = current_level.walls[cast_data.point - 1];	
			offset = calculate_wall_tex_offset(cast_data.side, cast_data.hit, dir, wall_sprite.surface -> w);

			const byte shade = 255 * calculate_shade((double) wall.h, cast_data.hit);
			SDL_SetTextureColorMod(wall_sprite.texture, shade, shade, shade);

			std_draw_floor(player, dir, raised_wall, cos_beta);

			first_draw_event = 0;
		}

		const SDL_Rect slice = {offset, 0, 1, wall_sprite.surface -> h};
		SDL_RenderCopyF(screen.renderer, wall_sprite.texture, &slice, &raised_wall);
	}
}

void raycast(const Player player, const double wall_y_shift, const double full_jump_height) {
	const double player_angle = to_radians(player.angle);

	for (int screen_x = 0; screen_x < settings.screen_width; screen_x += settings.ray_column_width) {
		const double theta = atan((screen_x - settings.half_screen_width) / settings.proj_dist) + player_angle;

		const VectorF dir = {cos(theta), sin(theta)};

		// begin DDA
		byte first_wall_hit = 1;
		double smallest_wall_y = DBL_MAX;
		DataDDA dda_data = init_dda(player.pos, dir);

		while (1) {
			iter_dda(&dda_data);

			const VectorI curr_tile = dda_data.curr_tile;
			if (VectorI_out_of_bounds(curr_tile)) break;

			const byte point = map_point(current_level.wall_data, curr_tile.x, curr_tile.y);
			if (point) {
				void (*fn) (const Player, const CastData, const int, const byte, double* const, const double,
					const double, const double, const double, const VectorF) = keys[SDL_SCANCODE_C] ? handle_ray : old_handle_ray;

				const CastData cast_data = {point, dda_data.side, dda_data.distance, VectorF_line_pos(player.pos, dir, dda_data.distance)};
				fn(player, cast_data, screen_x, first_wall_hit, &smallest_wall_y,
					player_angle, theta, wall_y_shift, full_jump_height, dir);
				first_wall_hit = 0;
			}
		}
		// end DDA
	}
}
