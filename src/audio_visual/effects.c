void death_effect(const vec pos, double* const p_height, double* const angle) {
	static byte red_fade = 1, black_fade = 0, r = 255, g = 255, b = 255;

	if (red_fade) {
		g--;
		b--;
		if (b == 0) red_fade = 0, black_fade = 1;
	}
	else if (black_fade) {
		if (--r == 0) black_fade = 0;
	}

	if (red_fade || black_fade) {
		SDL_SetTextureColorMod(screen.pixel_buffer, r, g, b);
		SDL_SetTextureColorMod(screen.shape_buffer, r, g, b);
	}

	const byte base_height = current_level.get_point_height(map_point(current_level.wall_data, pos[0], pos[1]), pos);
	if (*p_height > base_height -0.3) *p_height -= 0.003;
	(*angle) += 0.5;
}
