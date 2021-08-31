typedef SDL_Rect (*pos_and_size_fn_t) (void);
typedef void (*on_click_fn_t) (void);

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

void render_menu(const Menu* const menu) {
	const Color3 bg_color = menu -> bg_color, fg_color = menu -> fg_color;
	SDL_SetRenderDrawColor(screen.renderer, bg_color.r, bg_color.g, bg_color.b, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(screen.renderer);
	SDL_SetRenderDrawColor(screen.renderer, fg_color.r, fg_color.g, fg_color.b, SDL_ALPHA_OPAQUE);

	for (byte i = 0; i < menu -> textbox_count; i++) {
		Textbox* const textbox = &menu -> textboxes[i];
		const SDL_Rect textbox_pos_and_size = textbox -> pos_and_size_fn();
		SDL_RenderDrawRect(screen.renderer, &textbox_pos_and_size);
		SDL_RenderCopy(screen.renderer, textbox -> rendered_text, NULL, &textbox_pos_and_size);
	}

	SDL_RenderPresent(screen.renderer);
}

//////////

SDL_Rect textbox_1_pos(void) {
	return (SDL_Rect) {20, 20, 160, 80};
}

SDL_Rect textbox_2_pos(void) {
	return (SDL_Rect) {settings.half_screen_width, settings.half_screen_height, 150, 50};
}

void textbox_1_on_click(void) {}
void textbox_2_on_click(void) {}

void menu_test(void) {
	const Menu menu = init_menu((Color3) {205, 92, 92}, (Color3) {135, 206, 235}, 2,
		textbox_1_pos, textbox_1_on_click, "Textbox 1",
		textbox_2_pos, textbox_2_on_click, "Textbox 2");

	byte testing_menu = 1;
	while (testing_menu) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) testing_menu = 0;
		}

		render_menu(&menu);
	}

	deinit_menu(&menu);
}
