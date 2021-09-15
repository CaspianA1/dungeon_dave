void init_screen(void) {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0)
		FAIL("Unable to launch Dungeon Dave: %s\n", SDL_GetError());

	SDL_SetHintWithPriority(SDL_HINT_RENDER_VSYNC, "1", SDL_HINT_OVERRIDE);
	// SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, "1", SDL_HINT_OVERRIDE);

	SDL_CreateWindowAndRenderer(settings.screen_width, settings.screen_height,
		WINDOW_RENDERER_FLAGS, &screen.window, &screen.renderer);

	SDL_SetWindowTitle(screen.window, "Dungeon Dave");

	screen.pixel_format = SDL_AllocFormat(PIXEL_FORMAT);
	init_SDL_framebuffers(settings.screen_width, settings.screen_height, 0);
}

void deinit_screen(void) {
	SDL_FreeFormat(screen.pixel_format);
	SDL_DestroyWindow(screen.window);
	SDL_DestroyRenderer(screen.renderer);

	SDL_DestroyTexture(screen.pixel_buffer);
	SDL_DestroyTexture(screen.shape_buffer);

	SDL_Quit();
}

inlinable void prepare_for_drawing(void) {
	clear_statemap(occluded_by_walls);

	SDL_SetRenderDrawColor(screen.renderer, 0, 0, 0, 0);
	SDL_RenderClear(screen.renderer); // clearing the window

	SDL_LockTexture(screen.pixel_buffer, NULL, &screen.pixels, &screen.pixel_pitch);
	SDL_SetRenderTarget(screen.renderer, screen.shape_buffer);
	SDL_RenderClear(screen.renderer); // cleaning the shape buffer
}

inlinable void draw_tilted(SDL_Texture* const buffer, const SDL_FRect* const dest_crop, const double tilt) {
	SDL_RenderCopyExF(screen.renderer, buffer, NULL, dest_crop, tilt, NULL, SDL_FLIP_NONE);
}

void refresh(const Player* const player) {
	SDL_UnlockTexture(screen.pixel_buffer);
	SDL_SetRenderTarget(screen.renderer, NULL);

	/*
	When rotating the image according to the tilt angle, there's some dead space on the edges,
	resulting in some triangles on each side.
		 /| If the image were rotated by ϴ degrees, the side B would be on the right of the screen.
		/ | By setting the destination crop on the Y-axis to B larger on the bottom and top, there
	   /  | is no dead space. A is the screen width, so B is tan(ϴ) * A. The same idea in principle
	  /   | applies to the triangles on the sides of the screen.
	 /    |
	ϴ_____| B
	   A
	*/

	const Domain tilt = player -> tilt;
	if (tilt.val >= -tilt.step - 0.01 && tilt.val <= tilt.step + 0.01) {
		SDL_RenderCopy(screen.renderer, screen.pixel_buffer, NULL, NULL); // copy everything?
		SDL_RenderCopy(screen.renderer, screen.shape_buffer, NULL, NULL);
	}

	else {
		const double tan_tilt = tan(fabs(to_radians(tilt.val)));
		const double
			x_crop_adjust = tan_tilt * settings.screen_width,
			y_crop_adjust = tan_tilt * settings.screen_height;

		/*
		const double new_virtual_scr_w = settings.screen_width - x_crop_adjust * 2.0;;
		settings.proj_dist = new_virtual_scr_w / 2.0 / tan(to_radians(settings.fov / 2.0));
		*/

		const SDL_FRect dest_crop = {
			-x_crop_adjust, -y_crop_adjust,
			settings.screen_width + x_crop_adjust * 2.0,
			settings.screen_height + y_crop_adjust * 2.0
		};

		draw_tilted(screen.pixel_buffer, &dest_crop, tilt.val);
		draw_tilted(screen.shape_buffer, &dest_crop, tilt.val);
	}

	void draw_hud_elements(const Player* const);
	draw_hud_elements(player);
	
	SDL_RenderPresent(screen.renderer);
}
