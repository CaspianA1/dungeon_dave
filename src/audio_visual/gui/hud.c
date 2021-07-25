#define toggledef(r, g, b, key)\
	static Toggle toggle = {r, g, b, 0, 0, key};\
	if (!update_toggle(&toggle)) return;

byte update_toggle(Toggle* const toggle) {
	const byte pressed_key = keys[toggle -> key];
	if (pressed_key && !toggle -> enabled_previously)
		toggle -> enabled = !toggle -> enabled;

	toggle -> enabled_previously = pressed_key;
	return toggle -> enabled;
}

inlinable void draw_minimap(const vec pos) {
	toggledef(0, 0, 255, KEY_TOGGLE_MINIMAP);

	const int
		width_scale = (double) settings.screen_width
			/ current_level.map_size.x / settings.minimap_scale,
		height_scale = (double) settings.screen_height
			/ current_level.map_size.y / settings.minimap_scale;

	SDL_Rect wall_tile = {0, 0, width_scale, height_scale};

	for (int map_x = 0; map_x < current_level.map_size.x; map_x++) {
		for (int map_y = 0; map_y < current_level.map_size.y; map_y++) {
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

inlinable void draw_crosshair(const int y_shift) {
	toggledef(255, 215, 0, KEY_TOGGLE_CROSSHAIR);

	const byte half_dimensions = settings.screen_width / 40, thickness = settings.screen_width / 200;
	const ivec center = {settings.half_screen_width, y_shift};

	SDL_SetRenderDrawColor(screen.renderer, toggle.r, toggle.g, toggle.b, SDL_ALPHA_OPAQUE);

	const SDL_Rect
		across = {center.x - half_dimensions, center.y, half_dimensions * 2 + thickness, thickness},
		down = {center.x, center.y - half_dimensions, thickness, half_dimensions * 2 + thickness};
	
	SDL_RenderFillRect(screen.renderer, &across);
	SDL_RenderFillRect(screen.renderer, &down);
}

inlinable void draw_hp(const double hp, const double init_hp) {
	toggledef(0, 0, 0, KEY_TOGGLE_HP_PERCENT);
	// w/ % sign at the end

	char percent_str[5]; // max 4 characters = 100% + null terminator
	sprintf(percent_str, "%d%%", (byte) (hp / init_hp * 100));
	const SDL_Color color = {toggle.r, toggle.g, toggle.b, SDL_ALPHA_OPAQUE};

	const int avg_dimensions = (settings.screen_width + settings.screen_height) / 2;
	TTF_Font* font = TTF_OpenFont(STD_GUI_FONT_PATH, avg_dimensions / 20);
	SDL_Surface* const surface = TTF_RenderText_Solid(font, percent_str, color);
	SDL_Texture* const texture = SDL_CreateTextureFromSurface(screen.renderer, surface);

	SDL_RenderCopy(screen.renderer, texture, NULL, NULL);
}

// these are drawn to the window because if it were to the shape buffer, they would be rotated
void draw_hud_elements(const Player* const player, const double y_shift) {
	draw_minimap(player -> pos);
	draw_crosshair(y_shift);
	draw_hp(player -> hp, player -> init_hp);
}
