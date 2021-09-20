enum {
	HUD, BackgroundHP, PlayerMinimapDot
} ColorID;

static const Color3 hud_colors[3] = {
	{255, 215, 0}, // yellow-orange (mostly yellow)
	{0, 0, 0}, // black
	{255, 0, 0} // red
};

//////////

inlinable void draw_minimap(const vec pos) {
	toggledef(KEY_TOGGLE_MINIMAP);

	const ivec tile_amts = current_level.map_size;
	const vec screen_size = {settings.screen_width, settings.screen_height};
	const vec minimap_size = screen_size / vec_fill(settings.minimap_scale);
	const vec scale = minimap_size / vec_from_ivec(tile_amts);

	const vec origin = screen_size - minimap_size;
	SDL_FRect wall_tile = {0.0, origin[1], scale[0], scale[1]};
	const Color3 color = hud_colors[HUD];

	for (int map_y = 0; map_y < tile_amts.y; map_y++, wall_tile.y += (float) scale[1], wall_tile.x = 0.0) {
		for (int map_x = 0; map_x < tile_amts.x; map_x++, wall_tile.x += (float) scale[0]) {

			const byte point = *map_point(current_level.wall_data, map_x, map_y);
			const byte point_height = current_level.get_point_height(point, (vec) {map_x, map_y});
			const double shade = 1.0 - (double) point_height / current_level.max_point_height;
			draw_shaded_frect(color, shade, &wall_tile);
		}
	}

	const vec dot_pos = pos * scale;
	const SDL_FRect player_dot = {dot_pos[0], dot_pos[1] + origin[1], scale[0], scale[1]};
	draw_shaded_frect(hud_colors[PlayerMinimapDot], 1.0, &player_dot);
}

inlinable void draw_crosshair(void) {
	toggledef(KEY_TOGGLE_CROSSHAIR);

	const byte half_dimensions = settings.screen_width / 40, thickness = settings.screen_width / 200;
	const ivec center = {settings.half_screen_width, settings.half_screen_height};

	const Color3 color = hud_colors[HUD];
	SDL_SetRenderDrawColor(screen.renderer, color.r, color.g, color.b, SDL_ALPHA_OPAQUE);

	const SDL_Rect
		across = {center.x - half_dimensions, center.y, half_dimensions * 2 + thickness, thickness},
		down = {center.x, center.y - half_dimensions, thickness, half_dimensions * 2 + thickness};
	
	SDL_RenderFillRect(screen.renderer, &across);
	SDL_RenderFillRect(screen.renderer, &down);
}

inlinable SDL_Texture* make_hp_text(const double hp, SDL_Texture* const* const texture) {
	char percent_str[5]; // max 4 characters = 100% + null terminator
	sprintf(percent_str, "%d%%", (byte) round(hp / INIT_HP * 100));

	if (texture != NULL) SDL_DestroyTexture(*texture);
	return make_texture_from_text(percent_str, hud_colors[HUD]);
}

inlinable void draw_hp(const double hp) {
	toggledef(KEY_TOGGLE_HP_PERCENT);

	static double last_hp = -1.0;
	static int last_screen_width = -1, last_screen_height = -1;
	if (last_screen_width != settings.screen_width || last_screen_height != settings.screen_height || last_hp != hp) {
		gui_resources.hp_texture = make_hp_text(hp, &gui_resources.hp_texture);
		last_hp = hp;
		last_screen_width = settings.screen_width;
		last_screen_height = settings.screen_height;
	}

	const vec screen_size = {settings.screen_width, settings.screen_height};
	const vec box_size = screen_size / vec_fill(settings.minimap_scale);
	SDL_FRect hp_box = {box_size[0], screen_size[1] - box_size[1], box_size[0], box_size[1]};

	const Color3 color = hud_colors[BackgroundHP];
	SDL_SetRenderDrawColor(screen.renderer, color.r, color.g, color.b, SDL_ALPHA_OPAQUE); // black
	SDL_RenderFillRectF(screen.renderer, &hp_box);
	SDL_RenderCopyF(screen.renderer, gui_resources.hp_texture, NULL, &hp_box);
}

// these are drawn to the window because if it were to the shape buffer, they would be rotated
void draw_hud_elements(const Player* const player) {
	if (player -> is_dead) return;
	draw_minimap(player -> pos);
	draw_hp(player -> hp);
	draw_crosshair();
}
