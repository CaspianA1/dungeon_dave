/*
https://medium.com/@trier.t/ray-casting-in-web-assembly-part-1-81662a341669
https://lodev.org/cgtutor/raycasting.html
https://www.permadi.com/tutorial/raycast/rayc7.html
https://www.playfuljs.com/a-first-person-engine-in-265-lines/
https://www.youtube.com/watch?v=NbSee-XM7WA (follow an official guide by javidx9)
*/


const CastData dda(const Vector pos, const Vector dir) {
	const double slope = dir.y / dir.x, inverse_slope = dir.x / dir.y;

	const Vector unit_step_size = { // vRayUnitStepSize
		sqrt(1.0 + slope * slope), sqrt(1.0 + inverse_slope * inverse_slope)
	};

	Vector
		current_tile = {floor(pos.x), floor(pos.y)}, // vMapCheck
		ray_step, // vStep
		ray_length; // vRayLength1D

	if (dir.x < 0)
		ray_step.x = -1.0,
		ray_length.x = (pos.x - current_tile.x) * unit_step_size.x;
	else
		ray_step.x = 1.0,
		ray_length.x = (current_tile.x + 1.0 - pos.x) * unit_step_size.x;

	if (dir.y < 0)
		ray_step.y = -1.0,
		ray_length.y = (pos.y - current_tile.y) * unit_step_size.y;
	else
		ray_step.y = 1.0,
		ray_length.y = (current_tile.y + 1.0 - pos.y) * unit_step_size.y;

	double distance = 0;
	while (1) {
		if (ray_length.x < ray_length.y)
			distance = ray_length.x,
			current_tile.x += ray_step.x,
			ray_length.x += unit_step_size.x;
		else
			distance = ray_length.y,
			current_tile.y += ray_step.y,
			ray_length.y += unit_step_size.y;

		const byte point = map[(int) current_tile.y][(int) current_tile.x];
		if (is_a_wall(point)) {
			const Vector prod = {dir.x * distance, dir.y * distance};
			const Vector sum = {prod.x + pos.x, prod.y + pos.y};
			const CastData cast_data = {point, distance, sum};
			return cast_data;
		}
	}
}

void raycast_2(const Player player) {
	const double rad_player_angle = to_radians(player.angle);

	unsigned int screen_x = 0;

	// theta must not have a constant step
	for (double theta = player.angle - half_fov; theta < player.angle + half_fov; theta += theta_step) {
		const double rad_theta = to_radians(theta);

		const Vector ray_direction = {cos(rad_theta), sin(rad_theta)};
		const CastData collision = dda(player.pos, ray_direction);

		const double cos_beta = cos(rad_player_angle - rad_theta);
		const double correct_dist = collision.dist * cos_beta;
		screen.z_buffer[screen_x] = correct_dist;

		const int wall_height = screen.projection_distance / correct_dist;

		const SDL_Rect wall = {
			screen_x,
			half_screen_height - wall_height / 2 + player.y_pitch.val,
			1, wall_height
		};

		SDL_Rect pace_wall = wall; pace_wall.y += player.pace.screen_offset;

		if (collision.point <= plain_wall_count)
			draw_untextured_wall(wall_height, collision.point, pace_wall);
		else
			draw_textured_wall(wall_height, collision.point, pace_wall, collision.hit.x, collision.hit.y);

		draw_floor(wall, player, ray_direction.x, ray_direction.y, cos_beta);

		screen_x++;
	}
	draw_sprites(player);
}
