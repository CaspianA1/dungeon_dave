/*
the user can change this:
- minimap size
- ray column width (range from 1 to 10)
- fov (range from 60 to 120)
- max fps (range from 10 to 200)
*/

InputStatus display_option_screen(void) {
	SDL_Event event;

	while (1) {
		const Uint32 before = SDL_GetTicks();

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_WINDOWEVENT:
					break;
			}
		}

		after_gui_event(before);
	}

	return Exit;	
}
