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

void draw_wall(const CastData cast_data, const VectorF dir,
	const SDL_FRect wall, const int slice_h, const double shade_h) {

	const Sprite wall_sprite = current_level.walls[cast_data.point - 1];
	const int offset = calculate_wall_tex_offset(cast_data, dir, wall_sprite.surface -> w);

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

		const byte point = map_point(current_level.wall_data, curr_tile.x, curr_tile.y);
		if (point)
			return (CastData) {point, side, distance, VectorF_line_pos(pos, dir, distance)};
	}
}

void raycast(const Player player, const double wall_y_shift, const double full_jump_height) {
	const double player_angle = to_radians(player.angle);

	for (int screen_x = 0; screen_x < settings.screen_width; screen_x += settings.ray_column_width) {
		const double theta = atan((screen_x - settings.half_screen_width)
							/ settings.proj_dist) + player_angle;

		const VectorF ray_direction = {cos(theta), sin(theta)};
		const CastData cast_data = dda(player.pos, ray_direction);

		const double cos_beta = cos(player_angle - theta);
		const double corrected_dist = cast_data.dist * cos_beta;
		update_z_buffer(screen_x, corrected_dist);

		const double wall_h = settings.proj_dist / corrected_dist;

		const SDL_FRect wall = {
			screen_x,
			wall_y_shift - wall_h / 2.0 + full_jump_height / corrected_dist,
			settings.ray_column_width, wall_h
		};

		std_draw_floor(player, ray_direction, wall, cos_beta);
		std_draw_ceiling(player, ray_direction, wall, cos_beta);
		draw_wall(cast_data, ray_direction, wall, -1, -1);
	}
}
