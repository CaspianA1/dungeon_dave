DataAnimation init_animation_data(const char* const path, const int frames_per_row,
	const int frames_per_col, const int frame_count, const int fps, const byte enable_mipmap) {

	const Sprite sprite = init_sprite(path, enable_mipmap);

	return (DataAnimation) {
		sprite, frames_per_row, frames_per_col,
		sprite.size.x / frames_per_row,
		sprite.size.y / frames_per_col,
		frame_count, 0, 1.0 / fps, 0
	};
}

inlinable void progress_frame_ind(DataAnimation* const animation_data, const int begin, const int end) {
	const double current_time = SDL_GetTicks() / 1000.0;
	const double time_delta = current_time - animation_data -> last_frame_time;

	if (time_delta >= animation_data -> secs_per_frame) {
		if (++animation_data -> frame_ind == end)
			animation_data -> frame_ind = begin;

		animation_data -> last_frame_time = current_time;
	}
}

inlinable void progress_animation_data_frame_ind(DataAnimation* const animation_data) {
	progress_frame_ind(animation_data, 0, animation_data -> frame_count);
}

inlinable void progress_enemy_frame_ind(EnemyInstance* const enemy) {
	int begin = 0;
	for (byte i = 0; i < enemy -> state; i++)
		begin += enemy -> animation_seg_lengths[i];

	const int end = begin + enemy -> animation_seg_lengths[enemy -> state];

	AnimatedBillboard* const animated_billboard = &enemy -> animated_billboard;
	if (enemy -> state == Dead && animated_billboard -> animation_data.frame_ind == end - 1) return;

	progress_frame_ind(&animated_billboard -> animation_data, begin, end);
}

inlinable ivec get_spritesheet_frame_origin(const DataAnimation* const animation_data) {
	const int y_ind = animation_data -> frame_ind / animation_data -> frames_per_row;
	const int x_ind = animation_data -> frame_ind - y_ind * animation_data -> frames_per_row;
	const ivec size = animation_data -> sprite.size;

	return (ivec) {
		(double) x_ind / animation_data -> frames_per_row * size.x,
		(double) y_ind / animation_data -> frames_per_col * size.y
	};
}

void animate_weapon(DataAnimation* const animation_data, const vec pos,
	const byte paces_sideways_on_use, const byte in_use, const int y_pitch, const double v) {

	#ifndef SHADING_ENABLED
	(void) pos;
	#endif

	const ivec frame_origin = get_spritesheet_frame_origin(animation_data);

	const SDL_Rect sheet_crop = {
		frame_origin.x, frame_origin.y,
		animation_data -> frame_w, animation_data -> frame_h
	};

	/* the reason the `paces_sideways_on_use` flag exists is because the whip animation has half-drawn parts
	that are shown when the animation is scrolled on the x-axis, so to avoid revealing that, the flags stops
	the undrawn part from being shown (by stopping x-axis scrolling) when it cycles its animation.
	`weapon_arc` gives a constant back-and-forth movement that goes wider when the player speed is higher.
	there's a fabs call for the y-component of screen_pos b/c otherwise, the underside - the undrawn bottom
	of the weapon - would be shown.
	*/

	static vec prev_pos = {-1.0, -1.0};
	static double x = 0.0;
	if (prev_pos[0] != pos[0] || prev_pos[1] != pos[1]) {
		x += log(v + 1.0) * 1.2; // if stuck in corner, you'll still have speed, so line above removes weapon movement
		if (x > two_pi) x = 0.0;
	}
	prev_pos = pos;

	const double weapon_arc = sin(x) * settings.screen_width / 15.0; // pulsating back-and-forth movement

	const SDL_Rect screen_pos = {
		(!paces_sideways_on_use && in_use) ? 0 : weapon_arc,
		fabs(weapon_arc) + ((y_pitch < 0) ? 0 : y_pitch),
		settings.screen_width, settings.screen_height
	};

	SDL_Texture* const texture = animation_data -> sprite.texture;

	#ifdef SHADING_ENABLED
	byte shade = 255 * calculate_shade(settings.screen_height, pos);
	if (shade < 70) shade = 70;
	SDL_SetTextureColorMod(texture, shade, shade, shade);
	#endif

	// renders to shape buffer
	SDL_RenderCopy(screen.renderer, texture, &sheet_crop, &screen_pos);
	if (in_use) progress_animation_data_frame_ind(animation_data);
}
