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
	const Color3 fg_color, bg_color; // foreground color is for the textbox
} Menu;

// variadic params: pos fn, on_click fn, text
Menu init_menu(const Color3 fg_color, const Color3 bg_color, const unsigned textbox_count, ...) {
	const Menu menu = {
		TTF_OpenFont(gui_font_path, settings.avg_dimensions / font_size_divisor),
		wmalloc(textbox_count * sizeof(Textbox)), textbox_count, fg_color, bg_color
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
	const Color3 bg_color = menu -> bg_color, fg_color = menu -> fg_color;
	SDL_SetRenderDrawColor(screen.renderer, bg_color.r, bg_color.g, bg_color.b, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(screen.renderer);

	// SDL_RenderCopy(screen.renderer, current_level.walls[0].texture, NULL, NULL); // mipmap appeared

	for (byte i = 0; i < menu -> textbox_count; i++) {
		Textbox* const textbox = &menu -> textboxes[i];
		const SDL_Rect box = textbox -> pos_and_size_fn();

		ivec mouse;
		SDL_GetMouseState(&mouse.x, &mouse.y);

		if (mouse.x >= box.x && mouse.x <= box.x + box.w && mouse.y >= box.y && mouse.y <= box.y + box.h) {
			SDL_SetRenderDrawColor(screen.renderer, 255 - fg_color.r, 255 - fg_color.g, 255 - fg_color.b, SDL_ALPHA_OPAQUE);
			if (event.type == SDL_MOUSEBUTTONDOWN) {
				if (textbox -> on_click_fn() == Exit) return Exit;
			}
		}
		else
			SDL_SetRenderDrawColor(screen.renderer, bg_color.r, bg_color.g, bg_color.b, SDL_ALPHA_OPAQUE);

		SDL_RenderFillRect(screen.renderer, &box);
		SDL_RenderCopy(screen.renderer, textbox -> rendered_text, NULL, &box);
	}

	return ProceedAsNormal;
}

//////////

SDL_Rect textbox_1_pos(void) {
	return (SDL_Rect) {0, 0, settings.half_screen_width >> 1, settings.half_screen_height >> 1};
}

SDL_Rect textbox_2_pos(void) {
	return (SDL_Rect) {settings.half_screen_width, settings.half_screen_height, settings.screen_width / 10, settings.screen_height / 10};
}

InputStatus textbox_1_on_click(void) {
	puts("Clicked textbox 1");
	return Exit;
}

InputStatus textbox_2_on_click(void) {
	puts("Clicked textbox 2");
	return ProceedAsNormal;
}

void menu_test(void) {
	const Menu menu = init_menu((Color3) {205, 92, 92}, (Color3) {135, 206, 235}, 2,
		textbox_1_pos, textbox_1_on_click, "Textbox 1",
		textbox_2_pos, textbox_2_on_click, "Textbox 2");

	SDL_ShowCursor(SDL_TRUE);

	byte testing_menu = 1;
	while (testing_menu) {
		const Uint32 before = SDL_GetTicks();

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) testing_menu = 0;
		}

		if (render_menu(&menu) == Exit) testing_menu = 0;

		byte after_gui_event(const Uint32);
		after_gui_event(before);
	}

	SDL_ShowCursor(SDL_FALSE);
	deinit_menu(&menu);
}
