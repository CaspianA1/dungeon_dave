typedef struct {
	TTF_Font* font;
	Sprite sprite;
	byte r, g, b, has_background;
	SDL_Rect pos;
} Message;

Message init_message(const char* const text,
	const byte r, const byte g, const byte b, const byte has_background) {

	const double avg_dimensions = (settings.screen_width + settings.screen_height) / 2.0;

	Message message = {
		TTF_OpenFont("../assets/fonts/dnd.ttf", avg_dimensions / 10.0),
		.r = r, .g = g, .b = b, .has_background = has_background
	};

	if (message.font == NULL) FAIL("Could not open a font: %s\n", SDL_GetError());

	message.sprite.surface = TTF_RenderText_Solid(message.font, text, (SDL_Color) {r, g, b, SDL_ALPHA_OPAQUE});
	message.sprite.texture = SDL_CreateTextureFromSurface(screen.renderer, message.sprite.surface);

	return message;
}

inlinable void draw_colored_rect(const byte r, const byte g,
	const byte b, const SDL_Rect* const rect) {

	SDL_SetRenderDrawColor(screen.renderer, r, g, b, SDL_ALPHA_OPAQUE);
	SDL_RenderFillRect(screen.renderer, rect);
}

inlinable void draw_message(const Message message) {
	if (message.has_background) {
		SDL_SetRenderDrawColor(screen.renderer, 255 - message.r,
			255 - message.g, 255 - message.b, SDL_ALPHA_OPAQUE);
		SDL_RenderFillRect(screen.renderer, &message.pos);
	}

	SDL_RenderCopy(screen.renderer, message.sprite.texture, NULL, &message.pos);
}

inlinable byte mouse_over_message(const Message message) {
	int mouse_x, mouse_y;
	SDL_GetMouseState(&mouse_x, &mouse_y);
	const SDL_Rect box = message.pos;

	return mouse_x >= box.x && mouse_x <= box.x + box.w
		&& mouse_y >= box.y && mouse_y <= box.y + box.h;
}

inlinable void deinit_message(Message message) {
	TTF_CloseFont(message.font);
	deinit_sprite(message.sprite);
}

inlinable void after_gui_event(double* const pace_max,
	int* const z_pitch, const int mouse_y, const Uint32 before) {

	SDL_RenderPresent(screen.renderer);
	update_screen_dimensions(pace_max, z_pitch, mouse_y);
	tick_delay(before);
}

InputStatus display_logo(double* const pace_max, int* const z_pitch, const int mouse_y) {
	while (1) {
		const Uint32 before = SDL_GetTicks();

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_CLOSE) return Exit;
					break;
				case SDL_KEYUP: case SDL_MOUSEBUTTONDOWN:
					return ProceedAsNormal;
			}
		}

		Sprite logo = init_sprite("../assets/logo.bmp");
		SDL_RenderCopy(screen.renderer, logo.texture, NULL, NULL);
		deinit_sprite(logo);
		after_gui_event(pace_max, z_pitch, mouse_y, before);
	}
}

InputStatus display_title_screen(double* const pace_max,
	int* const z_pitch, const int mouse_y) {

	const Sound title_track = init_sound("../assets/audio/themes/title.wav", 0);
	play_sound(title_track, 1);
	if (display_logo(pace_max, z_pitch, mouse_y) == Exit) return Exit;

	if (TTF_Init() == -1)
		FAIL("Unable to initialize the font library: %s", SDL_GetError());

	Message start;
	InputStatus title_screen_input;

	while (1) {
		const Uint32 before = SDL_GetTicks();

		start = init_message("Start!", 255, 99, 71, 0);
		start.pos = (SDL_Rect) {
				settings.half_screen_width - start.sprite.surface -> w / 2,
				settings.half_screen_height - start.sprite.surface -> h / 2,
				start.sprite.surface -> w,
				start.sprite.surface -> h
			};

		start.has_background = mouse_over_message(start);

		/////

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
						title_screen_input = Exit;
						goto exit_title_screen;
					}
					break;

				case SDL_MOUSEBUTTONDOWN: {
					if (start.has_background) {
						SDL_ShowCursor(SDL_FALSE);
						title_screen_input = ProceedAsNormal;
						goto exit_title_screen;
					}
				}
			}
		}

		const int border_thickness = settings.screen_height / 50;
		const int twice_border_thickness = border_thickness * 2;

		const SDL_Rect darker_center_rect = {
			border_thickness, border_thickness,
			settings.screen_width - twice_border_thickness,
			settings.screen_height - twice_border_thickness
		};

		draw_colored_rect(228, 29, 29, NULL);
		draw_colored_rect(139, 0, 0, &darker_center_rect);
		draw_message(start);
		deinit_message(start);
		after_gui_event(pace_max, z_pitch, mouse_y, before);
	}
	exit_title_screen:
		deinit_message(start);
		TTF_Quit();
		deinit_sound(title_track);
		return title_screen_input;
}
