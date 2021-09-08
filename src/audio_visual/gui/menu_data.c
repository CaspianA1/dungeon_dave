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
	play_sound(&title_track, 1);

	const Sprite logo = init_sprite("assets/logo.bmp", 0);

	const Menu start_screen = init_menu((Color3) {255, 99, 71}, (Color3) {139, 0, 0}, (Color3) {228, 29, 29}, 1,
		start_button_pos, start_button_on_click, "Start!");

	// next up: a noise for clicking things
	const InputStatus input = menu_loop(&start_screen, logo.texture);

	deinit_sprite(logo);
	deinit_menu(&start_screen);
	deinit_sound(&title_track);

	return input;
}
