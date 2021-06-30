typedef struct {
	const byte point, side;
	const double dist;
	const VectorF hit;
} CastData;

/*
extended cast data:
- player angle and angle of ray in radians - check
- distance of raycast - check
- cos beta - check
- the point that the raycast hit, and the position that the raycast hit - check
- the screen x - check
- wall y shift - check
- full jump height - check
- if the first wall y hit - check
- direction - check

typedef struct {
	const double player_angle, theta, cos_beta, dist, wall_y_shift, full_jump_height;
	const VectorF hit, dir;
	const byte point, first_wall_hit;
	const int screen_x;
} DataRaycast;
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

void handle_ray(const Player player, const CastData cast_data, const int screen_x,
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

	const Sprite wall_sprite = current_level.walls[cast_data.point - 1];
	const int
		max_sprite_h = wall_sprite.surface -> h,
		offset = calculate_wall_tex_offset(cast_data.side, cast_data.hit, dir, wall_sprite.surface -> w);

	if (first_wall_hit) update_z_buffer(screen_x, corrected_dist);

	for (byte i = 0, first_draw_event = 1; i < point_height; i++) {
		SDL_FRect raised_wall = wall;
		raised_wall.y -= wall.h * i;

		/* completely obscured: starts under the tallest wall so far. wouldn't be seen, but this is for additional speed. */
		if ((double) raised_wall.y >= *smallest_wall_y || raised_wall.y >= settings.screen_height)
			continue;

		int sprite_h = max_sprite_h;

		// partially obscured: bottom of wall somewhere in middle of tallest
		if ((double) (raised_wall.y + raised_wall.h) > *smallest_wall_y) {
			raised_wall.h = *smallest_wall_y - (double) raised_wall.y;
			sprite_h = ceil(max_sprite_h * (double) raised_wall.h / wall_h);
		}

		else if (i == 0) std_draw_floor(player, dir, raised_wall, cos_beta);

		if ((double) raised_wall.y < *smallest_wall_y) *smallest_wall_y = (double) raised_wall.y;

		if (first_draw_event) {
			const byte shade = 255 * calculate_shade((double) wall.h, cast_data.hit);
			SDL_SetTextureColorMod(wall_sprite.texture, shade, shade, shade);
			first_draw_event = 0;
		}

		const SDL_Rect slice = {offset, 0, 1, sprite_h};
		SDL_RenderCopyF(screen.renderer, wall_sprite.texture, &slice, &raised_wall);
	}
}

void raycast(const Player player, const double wall_y_shift, const double full_jump_height) {
	const double player_angle = to_radians(player.angle);

	for (int screen_x = 0; screen_x < settings.screen_width; screen_x += settings.ray_column_width) {
		const double theta = atan((screen_x - settings.half_screen_width) / settings.proj_dist) + player_angle;

		const VectorF dir = {cos(theta), sin(theta)};

		double smallest_wall_y = DBL_MAX;
		DataDDA ray = init_dda(player.pos, dir);

		while (iter_dda(&ray)) {
			const byte point = map_point(current_level.wall_data, ray.curr_tile.x, ray.curr_tile.y);
			if (point) {
				const CastData cast_data = {point, ray.side, ray.dist, VectorF_line_pos(player.pos, dir, ray.dist)};
				handle_ray(player, cast_data, screen_x, ray.first_hit, &smallest_wall_y,
					player_angle, theta, wall_y_shift, full_jump_height, dir);

				ray.first_hit = 0;
			}
		}
	}
}
