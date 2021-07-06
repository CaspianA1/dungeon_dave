

byte update_toggle(Toggle* const toggle) {
	const byte pressed_key = keys[toggle -> key];
	if (pressed_key && !toggle -> enabled_previously)
		toggle -> enabled = !toggle -> enabled;

	toggle -> enabled_previously = pressed_key;
	return toggle -> enabled;
}

void draw_minimap(const VectorF pos) {
	static Toggle toggle = {30, 144, 255, 0, 0, KEY_TOGGLE_MINIMAP};
	if (!update_toggle(&toggle)) return;

	const double
		width_scale = (double) settings.screen_width
			/ current_level.map_width / settings.minimap_scale,
		height_scale = (double) settings.screen_height
			/ current_level.map_height / settings.minimap_scale;

	SDL_FRect wall = {0.0, 0.0, width_scale, height_scale};

	for (int map_x = 0; map_x < current_level.map_width; map_x++) {
		for (int map_y = 0; map_y < current_level.map_height; map_y++) {
			const byte point = map_point(current_level.wall_data, map_x, map_y);
			const double shade = 1.0 -
				((double) current_level.get_point_height(point, (VectorF) {map_x, map_y})
				/ current_level.max_point_height);

			SDL_SetRenderDrawColor(screen.renderer, toggle.r * shade, toggle.g * shade,
				toggle.b * shade, SDL_ALPHA_OPAQUE);
			wall.x = map_x * width_scale;
			wall.y = map_y * height_scale;
			SDL_RenderFillRectF(screen.renderer, &wall);
		}
	}

	const SDL_FRect player_dot = {pos[0] * width_scale, pos[1] * height_scale, width_scale, height_scale};
	SDL_SetRenderDrawColor(screen.renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderFillRectF(screen.renderer, &player_dot);
}

void draw_crosshair(const int y_pitch) {
	static Toggle toggle = {255, 215, 0, 0, 0, KEY_TOGGLE_CROSSHAIR};
	if (!update_toggle(&toggle)) return;

	const byte half_dimensions = settings.screen_width / 40, thickness = settings.screen_width / 200;
	const VectorI center = {settings.half_screen_width, y_pitch};

	SDL_SetRenderDrawColor(screen.renderer, toggle.r, toggle.g, toggle.b, SDL_ALPHA_OPAQUE);

	const SDL_Rect
		across = {center.x - half_dimensions, center.y, half_dimensions * 2 + thickness, thickness},
		down = {center.x, center.y - half_dimensions, thickness, half_dimensions * 2 + thickness};
	
	SDL_RenderFillRect(screen.renderer, &across);
	SDL_RenderFillRect(screen.renderer, &down);
}
