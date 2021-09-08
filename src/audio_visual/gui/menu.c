typedef SDL_Rect (*pos_and_size_fn_t) (void);
typedef InputStatus (*on_click_fn_t) (void);

typedef struct {
	pos_and_size_fn_t pos_and_size_fn;
	on_click_fn_t on_click_fn;
	SDL_Texture* rendered_text;
} Textbox;

typedef struct {
	TTF_Font* const font;
	Textbox* const textboxes;
	const byte textbox_count;
	const Color3 fg_color, bg_color, border_color; // foreground color is for the textbox
} Menu;

// returns if the screen dimensions changed
byte after_gui_event(const Uint32 before) {
	SDL_RenderPresent(screen.renderer);
	const byte dimensions_changed = update_screen_dimensions();
	tick_delay(before);
	return dimensions_changed;
}

// variadic params: pos fn, on_click fn, text
Menu init_menu(const Color3 fg_color, const Color3 bg_color, const Color3 border_color, const unsigned textbox_count, ...) {
	const Menu menu = {
		TTF_OpenFont(gui_font_path, settings.avg_dimensions / font_size_divisor),
		wmalloc(textbox_count * sizeof(Textbox)), textbox_count, fg_color, bg_color, border_color
	};

	va_list textbox_data;
	va_start(textbox_data, textbox_count);

	for (byte i = 0; i < textbox_count; i++) {
		Textbox* const textbox = &menu.textboxes[i];
		textbox -> pos_and_size_fn = va_arg(textbox_data, pos_and_size_fn_t);
		textbox -> on_click_fn = va_arg(textbox_data, on_click_fn_t);

		SDL_Surface* const surface = TTF_RenderText_Solid(
			menu.font, va_arg(textbox_data, const char*),
			(SDL_Color) {fg_color.r, fg_color.g, fg_color.b, SDL_ALPHA_OPAQUE});

		textbox -> rendered_text = SDL_CreateTextureFromSurface(screen.renderer, surface);

		SDL_FreeSurface(surface);
	}

	va_end(textbox_data);

	return menu;
}

void deinit_menu(const Menu* const menu) {
	Textbox* const textboxes = menu -> textboxes;
	for (byte i = 0; i < menu -> textbox_count; i++)
		SDL_DestroyTexture(textboxes[i].rendered_text);

	wfree(textboxes);
	TTF_CloseFont(menu -> font);
}

InputStatus render_menu(const Menu* const menu) {
	const Color3 bg_color = menu -> bg_color, fg_color = menu -> fg_color, border_color = menu -> border_color;

	SDL_SetRenderDrawColor(screen.renderer, border_color.r, border_color.g, border_color.b, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(screen.renderer);

	const int border_width = settings.avg_dimensions / 50;
	const int twice_border_width = border_width << 1;
	SDL_SetRenderDrawColor(screen.renderer, bg_color.r, bg_color.g, bg_color.b, SDL_ALPHA_OPAQUE);

	const SDL_Rect main_bg_rect = {
		border_width, border_width,
		settings.screen_width - twice_border_width,
		settings.screen_height - twice_border_width
	};

	SDL_RenderFillRect(screen.renderer, &main_bg_rect);

	for (byte i = 0; i < menu -> textbox_count; i++) {
		Textbox* const textbox = &menu -> textboxes[i];
		const SDL_Rect box = textbox -> pos_and_size_fn();

		ivec mouse;
		SDL_GetMouseState(&mouse.x, &mouse.y);

		if (mouse.x >= box.x && mouse.x <= box.x + box.w && mouse.y >= box.y && mouse.y <= box.y + box.h) {
			SDL_SetRenderDrawColor(screen.renderer, 255 - fg_color.r, 255 - fg_color.g, 255 - fg_color.b, SDL_ALPHA_OPAQUE);
			if (event.type == SDL_MOUSEBUTTONDOWN) {
				const InputStatus click_response = textbox -> on_click_fn();
				if (click_response != ProceedAsNormal) return click_response;
			}
		}

		else SDL_SetRenderDrawColor(screen.renderer, bg_color.r, bg_color.g, bg_color.b, SDL_ALPHA_OPAQUE);

		SDL_RenderFillRect(screen.renderer, &box);
		SDL_RenderCopy(screen.renderer, textbox -> rendered_text, NULL, &box);
	}

	return ProceedAsNormal;
}

InputStatus menu_loop(const Menu* const menu) {
	SDL_ShowCursor(SDL_TRUE);
	InputStatus input = ProceedAsNormal;
	byte done = 0;
	while (!done) {
			const Uint32 before = SDL_GetTicks();

			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_QUIT) {
					input = Exit;
					done = 1;
				}
			}

			const InputStatus menu_input = render_menu(menu);
			if (menu_input == Exit || menu_input == NextScreen) {
				if (menu_input == Exit) input = Exit;
				done = 1;
			}

			after_gui_event(before);
		}

	SDL_ShowCursor(SDL_FALSE);
	return input;
}
