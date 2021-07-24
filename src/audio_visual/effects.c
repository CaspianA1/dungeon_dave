void death_effect(const vec pos, double* const p_height, double* const angle, double* const tilt_val) {
	static byte red_fade = 1, black_fade_1 = 0, black_fade_2 = 0, r = 255, g = 255, b = 255;
	const byte color_step = 4, lowest_color = 43;

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

	const byte base_height = current_level.get_point_height(map_point(current_level.wall_data, pos[0], pos[1]), pos);
	const double bottom = base_height - 0.2 * (base_height + 1);
	const double dist_from_bottom = *p_height - bottom;

	if (*p_height > bottom) *p_height -= dist_from_bottom / 10.0;
	else *p_height = bottom;

	*angle += dist_from_bottom * 30.0;
	*tilt_val += 0.1;
}
