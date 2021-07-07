Message init_message(const char* const text,
	const byte r, const byte g, const byte b, const byte has_background) {

	const int avg_dimensions = (settings.screen_width + settings.screen_height) / 2;

	Message message = {
		TTF_OpenFont("../assets/dnd.ttf", avg_dimensions / 10.0),
		.r = r, .g = g, .b = b, .has_background = has_background
	};

	if (message.font == NULL) FAIL("Could not open a font: %s\n", SDL_GetError());

	message.sprite.surface = TTF_RenderText_Solid(message.font, text, (SDL_Color) {r, g, b, SDL_ALPHA_OPAQUE});
	message.sprite.texture = SDL_CreateTextureFromSurface(screen.renderer, message.sprite.surface);

	return message;
}

inlinable void draw_colored_rect(const byte r, const byte g, const byte b,
	const double shade, const SDL_Rect* const rect) {

	SDL_SetRenderDrawColor(screen.renderer, r * shade, g * shade, b * shade, SDL_ALPHA_OPAQUE);
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

InputStatus display_logo(void) {
	Sprite logo;
	InputStatus logo_input;
	byte displaying_logo = 1, dimensions_changed = 1;
	while (displaying_logo) {
		const Uint32 before = SDL_GetTicks();

		if (dimensions_changed) logo = init_sprite("../assets/logo.bmp");

		// SDL_PollEvent may loop more even after pressing exit, so this stops that
		byte checking_for_input = 1;
		while (SDL_PollEvent(&event) && checking_for_input) {
			switch (event.type) {
				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
						logo_input = Exit;
						displaying_logo = 0;
						checking_for_input = 0;
					}
					break;
				case SDL_KEYUP: case SDL_MOUSEBUTTONDOWN: {
					logo_input = ProceedAsNormal;
					displaying_logo = 0;
					checking_for_input = 0;
				}
			}
		}

		SDL_RenderCopy(screen.renderer, logo.texture, NULL, NULL);
		dimensions_changed = after_gui_event(before);

		if (dimensions_changed || !displaying_logo) deinit_sprite(logo);
	}
	return logo_input;
}

InputStatus display_title_screen(void) {
	const Sound title_track = init_sound("../assets/audio/themes/title.wav", 0);
	play_sound(title_track, 1);
	if (display_logo() == Exit) return Exit;

	if (TTF_Init() == -1)
		FAIL("Unable to initialize the font library: %s", SDL_GetError());

	Message start;
	InputStatus title_screen_input;
	byte displaying_title_screen = 1, dimensions_changed = 1;

	while (displaying_title_screen) {
		const Uint32 before = SDL_GetTicks();

		if (dimensions_changed) {
			start = init_message("Start!", 255, 99, 71, 0);
			start.pos = (SDL_Rect) {
					settings.half_screen_width - start.sprite.surface -> w / 2,
					settings.half_screen_height - start.sprite.surface -> h / 2,
					start.sprite.surface -> w,
					start.sprite.surface -> h
			};
		}

		start.has_background = mouse_over_message(start);

		/////

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
						title_screen_input = Exit;
						displaying_title_screen = 0;
					}
					break;

				case SDL_MOUSEBUTTONDOWN: {
					if (start.has_background) {
						SDL_ShowCursor(SDL_FALSE);
						title_screen_input = ProceedAsNormal;
						displaying_title_screen = 0;
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

		draw_colored_rect(228, 29, 29, 1.0, NULL);
		draw_colored_rect(139, 0, 0, 1.0, &darker_center_rect);
		draw_message(start);
		dimensions_changed = after_gui_event(before);
		if (dimensions_changed || !displaying_title_screen) deinit_message(start);
	}

	deinit_sound(title_track);
	return title_screen_input;
}
