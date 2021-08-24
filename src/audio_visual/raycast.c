typedef struct {
	double* const last_wall_y;
	const double player_angle, theta, dist, wall_y_shift, full_jump_height;
	const vec begin, hit, dir;
	// const double begin[2], hit[2], dir[2];
	const byte point, point_height, side, first_wall_hit;
	byte* const last_point_height;
	const int screen_x;
} DataRaycast;

typedef struct {
	double x, y, w, h;
} DRect;

inlinable int get_wall_tex_offset(const byte side, const vec hit, const vec dir, const int width) {
	const double component = hit[!side];
	const int offset = (component - (int) component) * width;
	const byte cond = side ? (dir[1] > 0.0) : (dir[0] < 0.0);
	return cond ? (width - 1) - offset : offset;
}

vec handle_ray(const DataRaycast* const d) {
	const double cos_beta = cos(d -> player_angle - d -> theta);
	const double corrected_dist = d -> dist * cos_beta;
	const double wall_h = settings.proj_dist / corrected_dist;

	const DRect wall_dest = {
		d -> screen_x,
		d -> wall_y_shift - (wall_h / 2.0) + (d -> full_jump_height / corrected_dist),
		settings.ray_column_width,
		wall_h
	};

	const double smallest_wall_y = wall_dest.y - (wall_h * (d -> point_height - 1)); // = wall_top

	if (d -> first_wall_hit) update_val_buffers(d -> screen_x, smallest_wall_y,
		wall_dest.y + wall_dest.h, corrected_dist, cos_beta, d -> dir);

	const Sprite wall_sprite = current_level.walls[d -> point - 1];
	const SDL_Rect mipmap_crop = get_mipmap_crop_from_wall(&wall_sprite, wall_h);
	const int max_sprite_h = mipmap_crop.h;

	SDL_Rect slice = {
		.x = get_wall_tex_offset(d -> side, d -> hit, d -> dir, mipmap_crop.w) + mipmap_crop.x,
		.y = mipmap_crop.y, .w = 1
	};

	#ifdef SHADING_ENABLED
	const byte shade = 255 * calculate_shade(wall_h, d -> hit);
	SDL_SetTextureColorMod(wall_sprite.texture, shade, shade, shade);
	#endif

	for (byte i = *d -> last_point_height; i < d -> point_height; i++) {
		DRect raised_wall_dest = wall_dest;
		raised_wall_dest.y -= wall_dest.h * i;

		// completely obscured: starts under the tallest wall so far; shouldn't be seen
		if (raised_wall_dest.y >= *d -> last_wall_y || raised_wall_dest.y >= settings.screen_height)
			continue;

		// partially obscured: bottom of wall somewhere in middle of tallest
		else if (raised_wall_dest.y + raised_wall_dest.h > *d -> last_wall_y) {
			raised_wall_dest.h = *d -> last_wall_y - raised_wall_dest.y;
			slice.h = ceil(max_sprite_h * raised_wall_dest.h / wall_h);
		}
		else slice.h = max_sprite_h;

		*d -> last_wall_y = raised_wall_dest.y;
		SDL_FRect frect = {raised_wall_dest.x, raised_wall_dest.y, raised_wall_dest.w, raised_wall_dest.h};
		SDL_RenderCopyF(screen.renderer, wall_sprite.texture, &slice, &frect);
	}
	*d -> last_point_height = d -> point_height;
	return (vec) {smallest_wall_y, wall_h};
}

// figure out why there is no red when the player height is 0
void draw_at_height_change(const int screen_x, const double last_wall_top, const double curr_wall_bottom, const byte point_height) {
	// from curr wall bottom (farther away) to last wall top

	if (last_wall_top < curr_wall_bottom) return;

	void draw_colored_rect(const byte, const byte, const byte, const double, const SDL_Rect* const);


	// if (screen_x == 200) {
	if (1) {
		SDL_SetRenderDrawColor(screen.renderer, (point_height == 1) ? 120 : 255, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderDrawLine(screen.renderer, screen_x, curr_wall_bottom, screen_x, last_wall_top);

		const SDL_Rect curr_wall_bottom_dot = {screen_x, curr_wall_bottom, 5, 5};
		draw_colored_rect(255, 165, 0, 1.0, &curr_wall_bottom_dot);

		const SDL_Rect last_wall_top_dot = {screen_x, last_wall_top, 5, 5};
		draw_colored_rect(0, 255, 0, 1.0, &last_wall_top_dot);
	}
}

void raycast(const Player* const player, const double wall_y_shift, const double full_jump_height) {
	const double player_angle = to_radians(player -> angle);

	for (int screen_x = 0; screen_x < settings.screen_width; screen_x += settings.ray_column_width) {
		const double theta = atan((screen_x - settings.half_screen_width) / settings.proj_dist) + player_angle;
		const vec dir = {cos(theta), sin(theta)};

		double last_wall_y = DBL_MAX, last_height_change_y = settings.screen_height;
		byte at_first_hit = 1, curr_point_height = player -> jump.height, last_point_height = player -> jump.height;
		DataDDA ray = init_dda((double[2]) UNPACK_2(player -> pos), (double[2]) UNPACK_2(dir), 1.0);

		while (iter_dda(&ray)) {
			const byte point = map_point(current_level.wall_data, ray.curr_tile[0], ray.curr_tile[1]);
			const vec hit = vec_line_pos(player -> pos, dir, ray.dist);
			const byte point_height = current_level.get_point_height(point, hit);

			if (point_height != curr_point_height) {
				double height_change_y, height_change_h;
				if (point) {
					const DataRaycast raycast_data = {
						&last_wall_y, player_angle, theta, ray.dist, wall_y_shift, full_jump_height,
						player -> pos, hit, dir, point, point_height, ray.side, at_first_hit, &last_point_height, screen_x
					};

					const vec wall_y_components = handle_ray(&raycast_data);
					height_change_y = wall_y_components[0], height_change_h = wall_y_components[1];
					at_first_hit = 0;
				}
				else {
					height_change_y = settings.screen_height, height_change_h = 0.0; // correct?
					last_point_height = 0;
				}

				// draw_at_height_change(screen_x, last_wall_y, height_change_y + height_change_h, point_height);

				curr_point_height = point_height;
				last_height_change_y = height_change_y;
			}
		}
	}
}
