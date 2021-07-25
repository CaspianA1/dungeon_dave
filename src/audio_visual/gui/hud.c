#define toggledef(r, g, b, key)\
	static Toggle toggle = {r, g, b, 0, 0, key};\
	if (!update_toggle(&toggle)) return;

static struct {
	TTF_Font* font;
	SDL_Texture* hp_texture;
} hud_resources = {NULL, NULL};

void deinit_hud_resources(void) {
	SDL_DestroyTexture(hud_resources.hp_texture);
	TTF_CloseFont(hud_resources.font);
}

byte update_toggle(Toggle* const toggle) {
	const byte pressed_key = keys[toggle -> key];
	if (pressed_key && !toggle -> enabled_previously)
		toggle -> enabled = !toggle -> enabled;

	toggle -> enabled_previously = pressed_key;
	return toggle -> enabled;
}

inlinable void draw_minimap(const vec pos) {
	toggledef(0, 0, 255, KEY_TOGGLE_MINIMAP);

	const ivec tile_amts = current_level.map_size;
	const vec screen_size = {settings.screen_width, settings.screen_height};
	const vec minimap_size = screen_size / vec_fill(settings.minimap_scale);
	const vec scale = minimap_size / vec_from_ivec(tile_amts);

	const vec origin = screen_size - minimap_size;
	SDL_FRect wall_tile = {0.0, origin[1], scale[0], scale[1]};

	for (int map_y = 0; map_y < tile_amts.y; map_y++, wall_tile.y += scale[1]) {
		for (int map_x = 0; map_x < tile_amts.x; map_x++, wall_tile.x += scale[0]) {

			const byte point = map_point(current_level.wall_data, map_x, map_y);
			const byte point_height = current_level.get_point_height(point, (vec) {map_x, map_y});
			const double shade = 1.0 - (double) point_height / current_level.max_point_height;
			draw_colored_frect(toggle.r, toggle.g, toggle.b, shade, &wall_tile);
		}
		wall_tile.x = 0.0;
	}

	const vec dot_pos = pos * scale;
	const SDL_FRect player_dot = {dot_pos[0], dot_pos[1] + origin[1], scale[0], scale[1]};
	draw_colored_frect(255, 0, 0, 1.0, &player_dot);
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

inlinable void make_hp_text(const byte r, const byte g, const byte b,
	const double hp, TTF_Font** const font, SDL_Texture** const texture) {

	char percent_str[5]; // max 4 characters = 100% + null terminator
	sprintf(percent_str, "%d%%", (byte) (hp / INIT_HP * 100));

	if (font != NULL) TTF_CloseFont(*font);
	*font = TTF_OpenFont(STD_GUI_FONT_PATH, settings.avg_dimensions / 10.0);

	SDL_Surface* const surface = TTF_RenderText_Solid(*font, percent_str, (SDL_Color) {r, g, b, SDL_ALPHA_OPAQUE});
	if (texture != NULL) SDL_DestroyTexture(*texture);
	*texture = SDL_CreateTextureFromSurface(screen.renderer, surface);
	SDL_FreeSurface(surface);
}

inlinable void draw_hp(const double hp) {
	toggledef(255, 0, 0, KEY_TOGGLE_HP_PERCENT);

	static double last_hp = -1.0;
	static int last_screen_width = -1, last_screen_height = -1;
	if (last_screen_width != settings.screen_width || last_screen_height != settings.screen_height || last_hp != hp) {
		make_hp_text(toggle.r, toggle.g, toggle.b, hp, &hud_resources.font, &hud_resources.hp_texture);
		last_hp = hp;
		last_screen_width = settings.screen_width;
		last_screen_height = settings.screen_height;
	}

	SDL_RenderCopy(screen.renderer, hud_resources.hp_texture, NULL, NULL);
}

// these are drawn to the window because if it were to the shape buffer, they would be rotated
void draw_hud_elements(const Player* const player, const double y_shift) {
	draw_minimap(player -> pos);
	draw_crosshair(y_shift);
	draw_hp(player -> hp);
}
