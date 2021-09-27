typedef SDL_Rect (*pos_and_size_fn_t) (void);
typedef InputStatus (*on_click_fn_t) (void);

typedef struct {
	pos_and_size_fn_t pos_and_size_fn;
	on_click_fn_t on_click_fn;
	SDL_Texture* rendered_text;
} Textbox;

typedef struct {
	Textbox* const textboxes;
	const byte textbox_count;
	const Color3 text_color, main_color, border_color;
} Menu;

// variadic params: pos fn, on_click fn, text
Menu init_menu(const Color3 text_color, const Color3 main_color, const Color3 border_color, const unsigned textbox_count, ...) {
	const Menu menu = {
		wmalloc(textbox_count * sizeof(Textbox)), textbox_count, text_color, main_color, border_color
	};

	va_list textbox_data;
	va_start(textbox_data, textbox_count);

	for (byte i = 0; i < textbox_count; i++) {
		Textbox* const textbox = &menu.textboxes[i];
		textbox -> pos_and_size_fn = va_arg(textbox_data, pos_and_size_fn_t);
		textbox -> on_click_fn = va_arg(textbox_data, on_click_fn_t);
		textbox -> rendered_text = make_texture_from_text(va_arg(textbox_data, const char*), text_color);
	}

	va_end(textbox_data);

	return menu;
}

void deinit_menu(const Menu* const menu) {
	Textbox* const textboxes = menu -> textboxes;
	for (byte i = 0; i < menu -> textbox_count; i++)
		SDL_DestroyTexture(textboxes[i].rendered_text);

	wfree(textboxes);
}

InputStatus render_menu(const Menu* const menu, const byte mouse_ready) {
	const Color3 main_color = menu -> main_color, text_color = menu -> text_color;

	/* This is done instead of a SDL_RenderClear call because there's an odd Metal bug
	where the background becomes corrupted when clearing with a whole color in full-screen mode. */
	const SDL_Rect background = {0, 0, settings.screen_width, settings.screen_height};
	draw_colored_rect(menu -> border_color, &background);

	//////////
	const int border_width = settings.avg_dimensions / 50;
	const int twice_border_width = border_width << 1;

	const SDL_Rect main_color_rect = {
		border_width, border_width,
		settings.screen_width - twice_border_width,
		settings.screen_height - twice_border_width
	};

	draw_colored_rect(main_color, &main_color_rect);
	//////////

	const Color3 inverse_text_color = {255 - text_color.r, 255 - text_color.g, 255 - text_color.b};

	for (byte i = 0; i < menu -> textbox_count; i++) {
		Textbox* const textbox = &menu -> textboxes[i];
		const SDL_Rect box = textbox -> pos_and_size_fn();

		ivec mouse;
		SDL_GetMouseState(&mouse.x, &mouse.y);

		if (mouse.x >= box.x && mouse.x <= box.x + box.w && mouse.y >= box.y && mouse.y <= box.y + box.h) {
			draw_colored_rect(inverse_text_color, &box);

			if (mouse_ready && event.type == SDL_MOUSEBUTTONDOWN) {
				play_sound(&gui_resources.sound_on_click);
				const InputStatus click_response = textbox -> on_click_fn();
				if (click_response != ProceedAsNormal) return click_response;
			}
		}

		SDL_RenderCopy(screen.renderer, textbox -> rendered_text, NULL, &box); // render text
	}

	return ProceedAsNormal;
}

InputStatus menu_loop(const Menu* const menu, SDL_Texture* const image_before_menu) {
	SDL_SetRelativeMouseMode(SDL_FALSE);

	InputStatus input = ProceedAsNormal;
	byte drawing_image = image_before_menu != NULL;
	byte mouse_down_after_image = !drawing_image, mouse_up_after_image = !drawing_image, done = 0;

	while (!done) {
		const Uint32 before = SDL_GetTicks();

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT:
					input = Exit;
					done = 1;
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (drawing_image) mouse_down_after_image = 1;
					break;
				case SDL_MOUSEBUTTONUP:
					if (drawing_image && mouse_down_after_image) {
						drawing_image = 0;
						mouse_up_after_image = 1;
					}
			}
		}

		if (drawing_image) SDL_RenderCopy(screen.renderer, image_before_menu, NULL, NULL);
		else {
			const InputStatus menu_input = render_menu(menu, mouse_down_after_image && mouse_up_after_image && !drawing_image);
			if (menu_input == Exit || menu_input == NextScreen) {
				if (menu_input == Exit) input = Exit;
				done = 1;
			}
		}

		after_gui_event(before);
	}

	SDL_SetRelativeMouseMode(SDL_TRUE);
	return input;
}
