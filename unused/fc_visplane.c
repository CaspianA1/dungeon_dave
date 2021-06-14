/* https://en.wikipedia.org/wiki/Doom_engine (visplane-based) */

/*
iteration 1: per column, pixel-wise (column-wise is not cache-friendly, pixel-wise is slow)
iteration 2: per column, pixel-wise, SIMD (last attribute speeds it up a bit)
iteration 3: per row, pixel-wise, plenty of overdraw (row-wise is cache-friendly,
	pixel-wise is slow, overdraw is slow)
iteration 4: per row, row-wise, not sure about overdraw (if no overdraw occurs, this would be ideal)

is this possible per row, row-wise?
*/

// offset is horizontal
void draw_row(const SDL_Surface* surface, SDL_Texture* texture, const int y_offset,
	const int screen_x, const int dest_width, const int screen_y, const double angle) {

	const static SDL_FPoint corner = {0, 0};
	// rotate src, not dest
	const SDL_Rect src = {0, y_offset, surface -> w, 1};
	const SDL_FRect dest = {screen_x, screen_y, dest_width, settings.ray_column_width};
	SDL_RenderCopyExF(screen.renderer, texture, &src, &dest, angle, &corner, SDL_FLIP_NONE);
}

void row_test(const Player player) {
	const Sprite sprite = current_level.walls[0];
	const SDL_Surface* surface = sprite.surface;
	SDL_Texture* texture = sprite.texture;
	const int start_x = 0, dest_width = 100;
	SDL_SetTextureColorMod(sprite.texture, 255, 255, SDL_ALPHA_OPAQUE);

	for (int y = 0; y < 200; y++) {
		const int offset = ((double) y / 200) * sprite.surface -> h;
		draw_row(surface, texture, offset, start_x, dest_width, y, player.angle);
	}
}

inlinable int get_row_4(const double y, const int z_pitch) {
	return y - settings.half_screen_height - z_pitch + 1;
}

// can a visplane be drawn completely horizontally (i.e. is the texture spacing even?)
void row_test_2(const Player player) {
	/* steps:
	1. find an intersection with the floor for a Y coordinate
	2. find the distance between that intersection
	3. draw the floor row according to that distance

	don't worry about screen_x; it will get drawn completely horizontally
	*/

	const double
		angle = player.angle,
		// angle = 0.0,
		p_height_ratio = player.jump.height * settings.screen_height / settings.proj_dist;

	const double
		opp_h = 0.5 + p_height_ratio,
		y_change = player.pace.screen_offset,
		// y_begin = settings.half_screen_height,
		y_begin = 0,
		y_end = settings.screen_height,
		rad_angle = to_radians(angle);

	const VectorF dir = {cos(rad_angle), sin(rad_angle)};
	byte** tex_hit_data = current_level.floor_data;

	const int screen_x = 100, dest_width = 200;

	for (int y = y_begin - y_change; y < y_end - y_change; y++) {
		const double actual_dist = opp_h / get_row_4(y, player.z_pitch) * settings.proj_dist;
		const VectorF hit = VectorF_line_pos(player.pos, dir, actual_dist);

		if (hit[0] < 0.0 || hit[0] >= current_level.map_width
			|| hit[1] < 0.0 || hit[1] >= current_level.map_height)
			continue;

		const byte point = tex_hit_data[(int) hit[1]][(int) hit[0]];

		const Sprite sprite = current_level.walls[point - 1];
		const SDL_Surface* surface = sprite.surface;
		SDL_Texture* texture = sprite.texture;

		const int y_offset = (hit[0] - (int) hit[0]) * (surface -> h - 1);
		draw_row(surface, texture, y_offset, screen_x, dest_width, y + y_change, angle);
	}
}
