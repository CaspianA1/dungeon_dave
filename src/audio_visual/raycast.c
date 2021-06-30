typedef struct {
	const byte point, side;
	const double dist;
	const VectorF hit;
} CastData;

/*
extended cast data:
- player angle and angle of ray in radians - check
- distance of raycast - check
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

typedef struct {
	double* const smallest_wall_y;
	const double player_angle, theta, dist, wall_y_shift, full_jump_height;
	const VectorF begin, hit, dir;
	const byte point, side, first_wall_hit;
	const int screen_x;
} DataRaycast;

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

// passing the player is temporary
void handle_ray(const DataRaycast d, const Player player) {
	const double cos_beta = cos(d.player_angle - d.theta);
	const double corrected_dist = d.dist * cos_beta;
	const double wall_h = settings.proj_dist / corrected_dist;
	const byte point_height = current_level.get_point_height(d.point, d.hit);

	const SDL_FRect wall = {
		d.screen_x,
		d.wall_y_shift - wall_h / 2.0 + d.full_jump_height / corrected_dist,
		settings.ray_column_width,
		wall_h
	};

	const Sprite wall_sprite = current_level.walls[d.point - 1];
	const int
		max_sprite_h = wall_sprite.surface -> h,
		offset = calculate_wall_tex_offset(d.side, d.hit, d.dir, wall_sprite.surface -> w);

	if (d.first_wall_hit) update_z_buffer(d.screen_x, corrected_dist);

	for (byte i = 0, first_draw_event = 1; i < point_height; i++) {
		SDL_FRect raised_wall = wall;
		raised_wall.y -= wall.h * i;

		/* completely obscured: starts under the tallest wall so far. wouldn't be seen, but this is for additional speed. */
		if ((double) raised_wall.y >= *d.smallest_wall_y || raised_wall.y >= settings.screen_height)
			continue;

		int sprite_h = max_sprite_h;

		// partially obscured: bottom of wall somewhere in middle of tallest
		if ((double) (raised_wall.y + raised_wall.h) > *d.smallest_wall_y) {
			raised_wall.h = *d.smallest_wall_y - (double) raised_wall.y;
			sprite_h = ceil(max_sprite_h * (double) raised_wall.h / wall_h);
		}

		else if (i == 0) std_draw_floor(d.begin, d.dir, player.pace.screen_offset, player.z_pitch,
			player.jump.height, cos_beta, raised_wall);

		if ((double) raised_wall.y < *d.smallest_wall_y) *d.smallest_wall_y = (double) raised_wall.y;

		if (first_draw_event) {
			const byte shade = 255 * calculate_shade((double) wall.h, d.hit);
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
				handle_ray((DataRaycast) {
					&smallest_wall_y, player_angle, theta, ray.dist, wall_y_shift, full_jump_height, player.pos,
					VectorF_line_pos(player.pos, dir, ray.dist), dir, point, ray.side, ray.at_first_hit, screen_x
				}, player);

				ray.at_first_hit = 0;
			}
		}
	}
}
