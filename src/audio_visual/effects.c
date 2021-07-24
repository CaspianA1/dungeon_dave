void death_effect(const vec pos, double* const p_height, double* const angle) {
	static byte red_fade = 1, black_fade_1 = 0, black_fade_2 = 0, r = 255, g = 255, b = 255;
	const byte color_step = 2, lowest_color = 43;

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
		if ((b -= color_step) == 1) black_fade_2 = 0;
	}

	else return;

	SDL_SetTextureColorMod(screen.pixel_buffer, r, g, b);
	SDL_SetTextureColorMod(screen.shape_buffer, r, g, b);

	const byte base_height = current_level.get_point_height(map_point(current_level.wall_data, pos[0], pos[1]), pos);
	const double min_base_height_offset = -0.28;
	if (*p_height > base_height + min_base_height_offset) {
		// const double 

		/*
		const double sink_speed = ((*p_height - min_base_height_offset) - base_height) / 10.0;
		*p_height -= sink_speed;
		*angle += sink_speed * 360.0;
		*/
	}

	DEBUG(*p_height, lf);
}
