typedef struct {
	Sprite text_sprite;
	byte selected;
	SDL_Rect pos;
} NewMessage;

typedef struct {
	const byte num_messages, has_sprite_background;
	const SDL_Color msg_color;

	NewMessage* const messages;
	TTF_Font* const font; // shared font

	union {
		Sprite sprite;
		struct {SDL_Color center, border;} whole_colors;
	} background;
} GUI;

// ../assets/dnd.ttf

// args: msg r, msg g, msg b, font name, sprite background, num messages, (bg sprite or bg colors), (text, {x, y, w, h})...
GUI init_gui(const byte msg_r, const byte msg_g, const byte msg_b,
	const char* const font_name, const byte has_sprite_background, const unsigned num_messages, ...) {
	va_list gui_data;
	va_start(gui_data, num_messages);

	const SDL_Color msg_color = {msg_r, msg_g, msg_b, SDL_ALPHA_OPAQUE};
	const int avg_dimensions = (settings.screen_width + settings.screen_height) / 2;

	GUI gui = {
		num_messages, has_sprite_background, msg_color,
		wmalloc(num_messages * sizeof(Message)), .font = TTF_OpenFont(font_name, avg_dimensions / 10.0)
	};

	if (has_sprite_background)
		gui.background.sprite = init_sprite(va_arg(gui_data, const char*));
	else {
		gui.background.whole_colors.center = (SDL_Color) {
			va_arg(gui_data, unsigned),
			va_arg(gui_data, unsigned),
			va_arg(gui_data, unsigned),
			SDL_ALPHA_OPAQUE
		};

		gui.background.whole_colors.border = (SDL_Color) {
			va_arg(gui_data, unsigned),
			va_arg(gui_data, unsigned),
			va_arg(gui_data, unsigned),
			SDL_ALPHA_OPAQUE
		};
	}

	for (byte i = 0; i < num_messages; i++) {
		SDL_Surface* const surface = TTF_RenderText_Solid(gui.font, va_arg(gui_data, const char*), msg_color);
		SDL_Texture* const texture = SDL_CreateTextureFromSurface(screen.renderer, surface);
		gui.messages[i] = (NewMessage) {{surface, texture}, 0, va_arg(gui_data, SDL_Rect)};
	}

	va_end(gui_data);
	return gui;
}

inlinable void deinit_gui(const GUI gui) {
	for (byte i = 0; i < gui.num_messages; i++)
		deinit_sprite(gui.messages[i].text_sprite);
	wfree(gui.messages);
	TTF_CloseFont(gui.font);
}

// returns if the screen dimensions changed
inlinable byte after_gui_event(const Uint32 before) {
	SDL_RenderPresent(screen.renderer);
	int fake_y_pitch;
	const byte dimensions_changed = update_screen_dimensions(&fake_y_pitch, settings.half_screen_height);
	tick_delay(before);
	return dimensions_changed;
}

void display_gui(const GUI gui) {
	byte displaying_gui = 1, dimensions_changed = 0;

	(void) gui;
	(void) dimensions_changed;

	while (displaying_gui) {
		const Uint32 before = SDL_GetTicks();
		after_gui_event(before);
	}
}
