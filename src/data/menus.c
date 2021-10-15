static const Color3
	menu_text_color_1 = {255, 99, 71}, // red/orange
	menu_text_color_2 = {255, 165, 0}, // orange
	menu_main_color = {139, 0, 0}, // darker red
	menu_border_color = {228, 29, 29}; // brighter red

//////////

SDL_Rect start_button_pos(void) {
	const ivec dimensions = {settings.avg_dimensions >> 2, settings.avg_dimensions / 10};
	return (SDL_Rect) {
		settings.half_screen_width - (dimensions.x >> 1),
		settings.half_screen_height - (dimensions.y >> 1),
		dimensions.x, dimensions.y
	};
}

InputStatus start_button_on_click(void) {
	return NextScreen;
}

InputStatus display_title_screen(void) {
	const Sound title_track = init_sound("assets/audio/themes/title.wav", 0);
	play_long_sound(&title_track);

	const Sprite logo = init_sprite("assets/logo.bmp", 0);

	const Menu start_screen = init_menu(menu_text_color_1, menu_main_color, menu_border_color, 1,
		start_button_pos, start_button_on_click, "Start!");

	const InputStatus input = menu_loop(&start_screen, logo.texture);

	deinit_sprite(logo);
	deinit_menu(&start_screen);
	deinit_sound(&title_track);

	return input;
}

////////// I NEED anonymous fns. Also, the reason for the pos fns is for dynamic screen resizing

inlinable ivec get_options_menu_button_size(void) {
	return (ivec) {settings.avg_dimensions >> 1, settings.avg_dimensions / 10};
}

SDL_Rect return_to_game_button_pos(void) {
	const ivec size = get_options_menu_button_size();
	return (SDL_Rect) {
		settings.half_screen_width - (size.x >> 1), size.y,
		size.x, size.y
	};
}

InputStatus return_to_game_button_on_click(void) {
	return NextScreen;
}

SDL_Rect detail_level_button_pos(void) {
	const ivec size = get_options_menu_button_size();
	return (SDL_Rect) {
		settings.half_screen_width - (size.x >> 1), size.y << 1,
		size.x, size.y
	};
}

InputStatus detail_level_button_on_click(void) {
	return ProceedAsNormal;
}

SDL_Rect mouse_sensitivity_button_pos(void) {
	const ivec size = get_options_menu_button_size();
	return (SDL_Rect) {
		settings.half_screen_width - (size.x >> 1), size.y * 3,
		size.x, size.y
	};
}

InputStatus mouse_sensitivity_button_on_click(void) {
	return ProceedAsNormal;
}

SDL_Rect sound_volume_button_pos(void) {
	const ivec size = get_options_menu_button_size();
	return (SDL_Rect) {
		settings.half_screen_width - (size.x >> 1), size.y << 2,
		size.x, size.y
	};
}

InputStatus sound_volume_button_on_click(void) {
	return ProceedAsNormal;
}

SDL_Rect fov_button_pos(void) {
	const ivec size = get_options_menu_button_size();
	return (SDL_Rect) {
		settings.half_screen_width - (size.x >> 1), size.y * 5,
		size.x, size.y
	};
}

InputStatus fov_button_on_click(void) {
	return ProceedAsNormal;
}

SDL_Rect max_fps_button_pos(void) {
	const ivec size = get_options_menu_button_size();
	return (SDL_Rect) {
		settings.half_screen_width - (size.x >> 1), size.y * 6,
		size.x, size.y
	};
}

InputStatus max_fps_button_on_click(void) {
	return ProceedAsNormal;
}

InputStatus display_options_menu(void) { // text color, main color, border color
	const Menu start_screen = init_menu(menu_text_color_2, menu_main_color, menu_border_color, 6,
		return_to_game_button_pos, return_to_game_button_on_click,       "| Return To Game    |",
		detail_level_button_pos, detail_level_button_on_click,           "| Detail Level      |",
		mouse_sensitivity_button_pos, mouse_sensitivity_button_on_click, "| Mouse Sensitivity |",
		sound_volume_button_pos, sound_volume_button_on_click,           "| Sound Volume      |",
		fov_button_pos, fov_button_on_click,                             "| Field of View     |",
		max_fps_button_pos, max_fps_button_on_click,                     "| Max FPS           |");

	const InputStatus input = menu_loop(&start_screen, NULL);
	deinit_menu(&start_screen);
	return input;
}
