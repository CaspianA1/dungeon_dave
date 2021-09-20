void init_gui_resources(void) {
	gui_resources.font = TTF_OpenFont(gui_font_path, settings.avg_dimensions / font_size_divisor);
	gui_resources.sound_on_click = init_sound(menu_click_sound_path, 1);
}

void deinit_gui_resources(void) {
	if (gui_resources.hp_texture != NULL) SDL_DestroyTexture(gui_resources.hp_texture);
	TTF_CloseFont(gui_resources.font);
	deinit_sound(&gui_resources.sound_on_click);
}

// returns if the toggle is set
byte update_toggle(Toggle* const toggle) {
	const byte pressed_key = keys[toggle -> key];
	if (pressed_key && !toggle -> enabled_previously)
		toggle -> enabled = !toggle -> enabled;

	toggle -> enabled_previously = pressed_key;
	return toggle -> enabled;
}

inlinable void draw_colored_rect(const Color3 color, const SDL_Rect* const rect) {
	SDL_SetRenderDrawColor(screen.renderer, color.r, color.g, color.b, SDL_ALPHA_OPAQUE);
	SDL_RenderFillRect(screen.renderer, rect);
}

inlinable void draw_shaded_frect(const Color3 color, const double shade, const SDL_FRect* const frect) {
	SDL_SetRenderDrawColor(screen.renderer, color.r * shade, color.g * shade, color.b * shade, SDL_ALPHA_OPAQUE);
	SDL_RenderFillRectF(screen.renderer, frect);
}

// returns if the screen dimensions changed
byte after_gui_event(const Uint32 before) {
	SDL_RenderPresent(screen.renderer);
	const byte dimensions_changed = update_screen_dimensions();
	tick_delay(before);
	return dimensions_changed;
}

SDL_Texture* make_texture_from_text(const char* const text, const Color3 color) {
	SDL_Surface* const surface = TTF_RenderText_Solid(gui_resources.font, text,
		(SDL_Color) {color.r, color.g, color.b, SDL_ALPHA_OPAQUE});

	SDL_Texture* const texture = SDL_CreateTextureFromSurface(screen.renderer, surface);
	SDL_FreeSurface(surface);
	return texture;
}
