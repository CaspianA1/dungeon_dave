byte death_effect(Player* const player) {
	static byte red_fade = 1, black_fade_1 = 0, black_fade_2 = 0, r = 255, g = 255, b = 255, first_call = 1;
	const byte color_step = 4, lowest_color = 43;

	if (first_call) {
		player -> body.v = 0.0;
		play_sound(&player -> sound_when_dying);
		first_call = 0;
	}

	if (red_fade) {
		g -= color_step;
		if ((b -= color_step) == lowest_color) red_fade = 0, black_fade_1 = 1;
	}
	else if (black_fade_1) {
		if ((r -= color_step) == lowest_color) black_fade_1 = 0, black_fade_2 = 1;
	}
	else if (black_fade_2) {
		r -= color_step;
		g -= color_step;
		if ((b -= color_step) <= 3) black_fade_2 = 0;
	}

	if (red_fade || black_fade_1 || black_fade_2) {
		SDL_SetTextureColorMod(screen.pixel_buffer, r, g, b);
		SDL_SetTextureColorMod(screen.shape_buffer, r, g, b);
	}
	else return 1; // returns 1 when the effect has finished

	const vec pos = player -> pos;
	double* const p_height = &player -> jump.height;

	const double lowest_fall_height = *map_point(current_level.heightmap, pos[0], pos[1]) - 0.45;
	const double dist_from_bottom = *p_height - lowest_fall_height;
	if (dist_from_bottom > 0.0) *p_height -= dist_from_bottom / 15.0;

	player -> angle += dist_from_bottom * 0.2;
	player -> tilt.val += 0.1;

	return 0;
}

void draw_skybox(const double p_angle, const double horizon_line) {
	const Skybox skybox = current_level.skybox;
	if (!skybox.enabled) return;
	const ivec max_size = skybox.sprite.size;

	const double turn_percent = p_angle / two_pi;
	const int
		src_col_index = turn_percent * max_size.x,
		src_width = max_size.x / 4.0;

	const int dest_y = 0, dest_height = horizon_line;
	const double look_up_percent = horizon_line / settings.screen_height;

	const int src_height = max_size.y * look_up_percent;
	const int src_y = max_size.y - src_height;

	const SDL_Rect src_1 = {src_col_index, src_y, src_width, src_height};
	SDL_Rect dest_1 =  {0, dest_y, settings.screen_width, dest_height};

	if (turn_percent > 0.75) {
		const double err_amt = (turn_percent - 0.75) * 4.0;
		const int src_error = max_size.x * err_amt, dest_error = settings.screen_width * err_amt;

		dest_1.w -= dest_error;

		const SDL_Rect
			src_2 = {0, src_y, src_error / 4, src_height},
			dest_2 = {dest_1.w, dest_y, settings.screen_width - dest_1.w, dest_height};

		SDL_RenderCopy(screen.renderer, skybox.sprite.texture, &src_2, &dest_2);
	}
	SDL_RenderCopy(screen.renderer, skybox.sprite.texture, &src_1, &dest_1);
}
