typedef struct {
	double* const last_wall_top;
	const double p_angle, p_height, theta, dist, horizon_line;
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

void handle_ray(const DataRaycast* const d, byte* const stop_from_tallest_wall,
	double* const last_projected_wall_top, double* const projected_wall_bottom) {

	const double cos_beta = cos(d -> p_angle - d -> theta);
	const double corrected_dist = d -> dist * cos_beta;
	const double wall_h = settings.proj_dist / corrected_dist;

	const DRect wall_dest = {
		d -> screen_x,
		get_projected_y(d -> horizon_line, wall_h * 0.5, wall_h, d -> p_height),
		settings.ray_column_width,
		wall_h
	};

	if (d -> first_wall_hit) update_val_buffer(wall_dest.x, corrected_dist, cos_beta, d -> dir);

	const Sprite wall_sprite = current_level.walls[d -> point - 1];
	const SDL_Rect mipmap_crop = get_mipmap_crop_from_wall(&wall_sprite, wall_h);
	const int max_sprite_h = mipmap_crop.h;

	SDL_Rect slice = {
		get_wall_tex_offset(d -> side, d -> hit, d -> dir, mipmap_crop.w) + mipmap_crop.x,
		mipmap_crop.y, .w = 1
	};

	#ifdef SHADING_ENABLED
	const byte shade = calculate_shade(wall_h, d -> hit);
	SDL_SetTextureColorMod(wall_sprite.texture, shade, shade, shade);
	#endif

	*last_projected_wall_top = *d -> last_wall_top;
	if (*last_projected_wall_top == DBL_MAX) *last_projected_wall_top = settings.screen_height - 1;

	double wall_dest_h_sum = 0.0;

	for (byte i = *d -> last_point_height; i < d -> point_height; i++) {
		DRect raised_wall_dest = wall_dest;
		raised_wall_dest.y -= wall_dest.h * i;

		// completely obscured: starts under the tallest wall so far; shouldn't be seen
		if (raised_wall_dest.y >= *d -> last_wall_top) continue;

		// partially obscured: bottom of wall somewhere in middle of tallest
		else if (raised_wall_dest.y + raised_wall_dest.h > *d -> last_wall_top) {
			raised_wall_dest.h = *d -> last_wall_top - raised_wall_dest.y;
			slice.h = ceil(max_sprite_h * raised_wall_dest.h / wall_h);
		}
		else slice.h = max_sprite_h;

		wall_dest_h_sum += raised_wall_dest.h;

		*d -> last_wall_top = raised_wall_dest.y;
		SDL_FRect frect = {raised_wall_dest.x, raised_wall_dest.y, raised_wall_dest.w, raised_wall_dest.h};
		SDL_RenderCopyF(screen.renderer, wall_sprite.texture, &slice, &frect);
	}

	//////////

	double proj_wall_top = *d -> last_wall_top;
	double proj_wall_bottom = proj_wall_top + wall_dest_h_sum;

	if (proj_wall_bottom >= settings.screen_height) proj_wall_bottom = settings.screen_height - 1;
	if (proj_wall_top < 0.0) proj_wall_top = 0.0;
	else if (proj_wall_top >= settings.screen_height) proj_wall_top = settings.screen_height - 1;

	for (int y = round(proj_wall_top); y < round(proj_wall_bottom); y++)
		set_statemap_bit(occluded_by_walls, wall_dest.x, y);

	*projected_wall_bottom = proj_wall_bottom;
	*stop_from_tallest_wall = d -> point_height == current_level.max_point_height && d -> p_height <= current_level.max_point_height - 0.5;
}

// once the colors are correct per vertical line, this will be done
void mark_floor(const DataRaycast* const d, const double last_projected_wall_top, const double projected_wall_bottom) {
	if (doubles_eq(last_projected_wall_top - projected_wall_bottom, 0.0)) return;
	const int x = d -> screen_x;
	SDL_SetRenderDrawColor(screen.renderer, 0, 255 / (*d -> last_point_height + 1), 0, SDL_ALPHA_OPAQUE);

	static double offset = 0.0;
	const double step = 0.001;
	if (keys[SDL_SCANCODE_T]) offset -= step;
	if (keys[SDL_SCANCODE_Y]) offset += step;
	if (keys[SDL_SCANCODE_U]) offset = 0.0;

	SDL_RenderDrawLineF(screen.renderer, x + offset, last_projected_wall_top, x, projected_wall_bottom); // last wall top -> curr wall bottom
}

void raycast(const Player* const player, const double horizon_line, const double p_height) {
	const vec p_pos = player -> pos;
	const double p_angle = to_radians(player -> angle);

	for (int screen_x = 0; screen_x < settings.screen_width; screen_x += settings.ray_column_width) {
		const double theta = atan((screen_x - settings.half_screen_width) / settings.proj_dist) + p_angle;
		const vec dir = {cos(theta), sin(theta)};

		double last_wall_top = DBL_MAX;

		byte at_first_hit = 1, last_point_height = current_level.get_point_height(
			*map_point(current_level.wall_data, p_pos[0], p_pos[1]), p_pos);

		DataDDA ray = init_dda((double[2]) UNPACK_2(player -> pos), (double[2]) UNPACK_2(dir), 1.0);

		while (iter_dda(&ray)) {
			const byte point = *map_point(current_level.wall_data, ray.curr_tile[0], ray.curr_tile[1]);
			const vec hit = vec_line_pos(p_pos, dir, ray.dist);
			const byte point_height = current_level.get_point_height(point, hit);

			if (last_point_height != point_height) {
				if (point) {
					const DataRaycast raycast_data = {
						&last_wall_top, p_angle, p_height, theta, ray.dist, horizon_line, p_pos,
						hit, dir, point, point_height, ray.side, at_first_hit, &last_point_height, screen_x
					};

					byte stop_from_tallest_wall;
					double last_projected_wall_top, projected_wall_bottom;

					handle_ray(&raycast_data, &stop_from_tallest_wall, &last_projected_wall_top, &projected_wall_bottom);
					// mark_floor(&raycast_data, last_projected_wall_top, projected_wall_bottom);
					if (stop_from_tallest_wall) break;

					at_first_hit = 0;
				}

				last_point_height = point_height;
			}
		}
	}
}
