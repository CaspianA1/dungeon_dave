const char* STD_GUI_FONT_PATH = "../assets/dnd.ttf";

typedef struct {
	const char* text;
	Sprite text_sprite;
	byte selected;
	SDL_Rect (*pos) (const Sprite);
} NewMessage;

typedef struct {
	const byte message_count, has_sprite_background, message_scale;
	const SDL_Color msg_color;

	NewMessage* const messages;
	TTF_Font* font; // shared font
	const char* const font_name;

	union {
		Sprite sprite;
		struct {SDL_Color center, border;} whole_colors;
	} background;
} GUI;

// args: msg r, msg g, msg b, font name, sprite bg, num messages, scale (bg sprite | bg colors), (text, {x, y, w, h})...
GUI init_gui(const byte msg_r, const byte msg_g, const byte msg_b, const char* const font_name,
	const byte has_sprite_background, const byte message_scale, const unsigned message_count, ...) {

	va_list gui_data;
	va_start(gui_data, message_count);

	const SDL_Color msg_color = {msg_r, msg_g, msg_b, SDL_ALPHA_OPAQUE};
	const int avg_dimensions = (settings.screen_width + settings.screen_height) / 2;

	GUI gui = {
		message_count, has_sprite_background, message_scale, msg_color,
		wmalloc(message_count * sizeof(Message)), TTF_OpenFont(font_name, avg_dimensions / message_scale),
		.font_name = font_name
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

	for (byte i = 0; i < message_count; i++) {
		const char* text = va_arg(gui_data, const char*); // or a msg?
		SDL_Surface* const surface = TTF_RenderText_Solid(gui.font, text, msg_color);
		SDL_Texture* const texture = SDL_CreateTextureFromSurface(screen.renderer, surface);
		gui.messages[i] = (NewMessage) {text, {surface, texture}, 0, va_arg(gui_data, SDL_Rect (*) (const Sprite))};
	}

	va_end(gui_data);
	return gui;
}

inlinable void deinit_gui(const GUI gui) {
	for (byte i = 0; i < gui.message_count; i++)
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

void resize_gui(GUI* const gui) {
	TTF_CloseFont(gui -> font);
	const int avg_dimensions = (settings.screen_width + settings.screen_height) / 2;
	gui -> font = TTF_OpenFont(gui -> font_name, avg_dimensions / gui -> message_scale);

	for (byte i = 0; i < gui -> message_count; i++) {
		NewMessage* message = &gui -> messages[i];
		deinit_sprite(message -> text_sprite);

		Sprite* text_sprite = &message -> text_sprite;
		text_sprite -> surface = TTF_RenderText_Solid(gui -> font, message -> text, gui -> msg_color);
		text_sprite -> texture = SDL_CreateTextureFromSurface(screen.renderer, text_sprite -> surface);
	}
}

InputStatus display_gui(GUI* const gui) {
	byte displaying_gui = 1;
	InputStatus user_input = Exit;

	while (displaying_gui) {
		const Uint32 before = SDL_GetTicks();

		// switch on input

		if (after_gui_event(before)) resize_gui(gui);
	}
	return user_input;
}
