byte update_toggle(Toggle* const toggle) {
	const byte pressed_key = keys[toggle -> key];
	if (pressed_key && !toggle -> enabled_previously)
		toggle -> enabled = !toggle -> enabled;

	toggle -> enabled_previously = pressed_key;
	return toggle -> enabled;
}

void draw_minimap(const vec pos) {
	static Toggle toggle = {30, 144, 255, 0, 0, KEY_TOGGLE_MINIMAP};
	if (!update_toggle(&toggle)) return;

	const int
		width_scale = (double) settings.screen_width
			/ current_level.map_width / settings.minimap_scale,
		height_scale = (double) settings.screen_height
			/ current_level.map_height / settings.minimap_scale;

	SDL_Rect wall_tile = {0, 0, width_scale, height_scale};

	for (int map_x = 0; map_x < current_level.map_width; map_x++) {
		for (int map_y = 0; map_y < current_level.map_height; map_y++) {
			const byte point = map_point(current_level.wall_data, map_x, map_y);
			const byte point_height = current_level.get_point_height(point, (vec) {map_x, map_y});
			const double shade = 1.0 - (double) point_height / current_level.max_point_height;

			wall_tile.x = map_x * width_scale;
			wall_tile.y = map_y * height_scale;
			draw_colored_rect(toggle.r, toggle.g, toggle.b, shade, &wall_tile);
		}
	}

	const SDL_Rect player_dot = {pos[0] * width_scale, pos[1] * height_scale, width_scale, height_scale};
	draw_colored_rect(255, 0, 0, 1.0, &player_dot);
}

void draw_crosshair(const int y_shift) {
	static Toggle toggle = {255, 215, 0, 0, 0, KEY_TOGGLE_CROSSHAIR};
	if (!update_toggle(&toggle)) return;

	const byte half_dimensions = settings.screen_width / 40, thickness = settings.screen_width / 200;
	const ivec center = {settings.half_screen_width, y_shift};

	SDL_SetRenderDrawColor(screen.renderer, toggle.r, toggle.g, toggle.b, SDL_ALPHA_OPAQUE);

	const SDL_Rect
		across = {center.x - half_dimensions, center.y, half_dimensions * 2 + thickness, thickness},
		down = {center.x, center.y - half_dimensions, thickness, half_dimensions * 2 + thickness};
	
	SDL_RenderFillRect(screen.renderer, &across);
	SDL_RenderFillRect(screen.renderer, &down);
}
