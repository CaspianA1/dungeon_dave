typedef struct {
	const byte point, side;
	const double dist;
	const VectorF hit;
} CastData;

void draw_minimap(const VectorF pos) {
	static byte minimap_enabled = 0;
	if (keys[KEY_DISABLE_MINIMAP]) minimap_enabled = 0;
	if (keys[KEY_ENABLE_MINIMAP]) minimap_enabled = 1;
	if (!minimap_enabled) return;

	const double
		width_scale = (double) settings.screen_width
			/ current_level.map_width / settings.minimap_scale,
		height_scale = (double) settings.screen_height
			/ current_level.map_height / settings.minimap_scale;

	SDL_SetRenderDrawColor(screen.renderer, 30, 144, 255, SDL_ALPHA_OPAQUE);

	SDL_FRect wall = {0.0, 0.0, width_scale, height_scale};

	for (int map_x = 0; map_x < current_level.map_width; map_x++) {
		for (int map_y = 0; map_y < current_level.map_height; map_y++) {
			if (wall_point(map_x, map_y)) {
				wall.x = map_x * width_scale;
				wall.y = map_y * height_scale;
				SDL_RenderFillRectF(screen.renderer, &wall);
			}
		}
	}

	const SDL_FRect player_dot = {pos[0] * width_scale, pos[1] * height_scale, 5, 5};
	SDL_SetRenderDrawColor(screen.renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderFillRectF(screen.renderer, &player_dot);
}

void draw_wall(const CastData cast_data, const VectorF dir,
	const SDL_FRect wall, const int slice_h, const double shade_h) {

	const Sprite wall_sprite = current_level.walls[cast_data.point - 1];
	const int max_offset = wall_sprite.surface -> w - 1;


	int offset;
	if (cast_data.side) { // before, I used `round` (why did I do that?)
		const int x_offset = (cast_data.hit[0] - floor(cast_data.hit[0])) * max_offset;
		offset = (dir[1] > 0.0) ? max_offset - x_offset : x_offset;
	}
	else {
		const int y_offset = (cast_data.hit[1] - floor(cast_data.hit[1])) * max_offset;
		offset = (dir[0] < 0.0) ? max_offset - y_offset : y_offset;
	}

	draw_column(wall_sprite, cast_data.hit, offset, slice_h, shade_h, &wall);
}

CastData dda(const VectorF pos, const VectorF dir) {
	const VectorF unit_step_size = {fabs(1.0 / dir[0]), fabs(1.0 / dir[1])};
	VectorF ray_length;
	VectorI curr_tile = VectorF_floor(pos), ray_step;
	byte side;

	if (dir[0] < 0.0)
		ray_step.x = -1,
		ray_length[0] = (pos[0] - curr_tile.x) * unit_step_size[0];
	else
		ray_step.x = 1,
		ray_length[0] = (curr_tile.x + 1.0 - pos[0]) * unit_step_size[0];

	if (dir[1] < 0.0)
		ray_step.y = -1,
		ray_length[1] = (pos[1] - curr_tile.y) * unit_step_size[1];
	else
		ray_step.y = 1,
		ray_length[1] = (curr_tile.y + 1.0 - pos[1]) * unit_step_size[1];

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

		const byte point = wall_point(curr_tile.x, curr_tile.y);
		if (point)
			return (CastData) {point, side, distance, VectorF_line_pos(pos, dir, distance)};
	}
}

void raycast(const Player player) {
	const double player_angle = to_radians(player.angle);

	for (int screen_x = 0; screen_x < settings.screen_width; screen_x += settings.ray_column_width) {
		const double theta = atan((screen_x - settings.half_screen_width)
							/ settings.proj_dist) + player_angle;

		const VectorF ray_direction = {cos(theta), sin(theta)};
		const CastData cast_data = dda(player.pos, ray_direction);

		const double cos_beta = cos(player_angle - theta);
		const double correct_dist = cast_data.dist * cos_beta;
		screen.z_buffer[screen_x] = correct_dist;

		const double wall_h = settings.proj_dist / correct_dist;

		const SDL_FRect wall = {
			screen_x,
			settings.half_screen_height - (wall_h / 2.0) +
				player.z_pitch + player.pace.screen_offset
				+ (player.jump.height * settings.screen_height / correct_dist) - 2.0,

			settings.ray_column_width,
			wall_h + 2.0 // without the `2.0`, a stitch appears
		};

		std_draw_floor(player, ray_direction, wall, cos_beta);
		std_draw_ceiling(player, ray_direction, wall, cos_beta);
		draw_wall(cast_data, ray_direction, wall, -1, -1);
	}
}
