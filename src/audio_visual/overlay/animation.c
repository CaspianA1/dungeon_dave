DataAnimationImmut init_immut_animation_data(const char* const path, const DrawableType drawable_type,
	const int frames_per_row, const int frames_per_col, const int frame_count, const int fps) {

	const Sprite sprite = init_sprite(path, drawable_type);

	return (DataAnimationImmut) {
		sprite, {frames_per_row, frames_per_col},
		{sprite.size.x / frames_per_row,
		sprite.size.y / frames_per_col},
		frame_count, 1.0 / fps
	};
}

inlinable byte progress_frame_ind(DataAnimation* const animation_data, const int begin, const int end) {
	const double current_time = SDL_GetTicks() / 1000.0;
	DataAnimationMut* const mut_animation_data = &animation_data -> mut;
	const double time_delta = current_time - mut_animation_data -> last_frame_time;

	byte animation_cycle_done = 0;
	if (time_delta >= animation_data -> immut.secs_per_frame) {
		if (++mut_animation_data -> frame_ind == end) {
			mut_animation_data -> frame_ind = begin;
			animation_cycle_done = 1;
		}
		mut_animation_data -> last_frame_time = current_time;
	}
	return animation_cycle_done;
}

inlinable byte progress_animation_data_frame_ind(DataAnimation* const animation_data) {
	return progress_frame_ind(animation_data, 0, animation_data -> immut.frame_count);
}

inlinable void progress_enemy_instance_frame_ind(EnemyInstance* const enemy_instance) {
	const Enemy* const enemy = enemy_instance -> enemy;
	const byte* const seg_lengths = enemy -> animation_seg_lengths;
	const EnemyState enemy_state = enemy_instance -> state;

	int begin = 0;
	for (byte i = 0; i < enemy_state; i++) begin += seg_lengths[i];

	const int end = begin + seg_lengths[enemy_state];

	DataAnimationMut* const mut_animation_data = &enemy_instance -> mut_animation_data;
	if (enemy_state == Dead && mut_animation_data -> frame_ind == end - 1) return;

	DataAnimation animation_data = {enemy -> animation_data, *mut_animation_data};
	progress_frame_ind(&animation_data, begin, end);
	*mut_animation_data = animation_data.mut; // Reassignment not expensive b/c struct is small
}

inlinable ivec get_spritesheet_frame_origin(const DataAnimation* const animation_data) {
	const DataAnimationImmut* const immut = &animation_data -> immut;
	const int frame_ind = animation_data -> mut.frame_ind;

	const ivec
		frames_per_axis = immut -> frames_per_axis,
		size = immut -> sprite.size;

	const int y_ind = frame_ind / frames_per_axis.x;
	const int x_ind = frame_ind - y_ind * frames_per_axis.x;

	return (ivec) {
		(double) x_ind / frames_per_axis.x * size.x,
		(double) y_ind / frames_per_axis.y * size.y
	};
}

void animate_weapon(DataAnimation* const animation_data, const vec pos,
	const byte paces_sideways_on_use, const byte in_use, const double v) {

	const ivec
		frame_origin = get_spritesheet_frame_origin(animation_data),
		frame_size = animation_data -> immut.frame_size;

	const SDL_Rect sheet_crop = {
		frame_origin.x, frame_origin.y,
		frame_size.x, frame_size.y
	};

	/* the reason the `paces_sideways_on_use` flag exists is because the whip animation has half-drawn parts
	that are shown when the animation is scrolled on the x-axis, so to avoid revealing that, the flags stops
	the undrawn part from being shown (by stopping x-axis scrolling) when it cycles its animation.
	`weapon_arc` gives a constant back-and-forth movement that goes wider when the player speed is higher.
	there's a fabs call for the y-component of screen_pos b/c otherwise, the underside - the undrawn bottom
	of the weapon - would be shown.
	*/

	static double x = 0.0;
	x += log(v + 1.0) * 1.2;
	if (x > two_pi) x = 0.0;

	const double weapon_arc = sin(x) * settings.screen_width / 15.0; // pulsating back-and-forth movement

	const SDL_Rect screen_pos = {
		(!paces_sideways_on_use && in_use) ? 0 : weapon_arc,
		fabs(weapon_arc),
		settings.screen_width, settings.screen_height
	};

	SDL_Texture* const texture = animation_data -> immut.sprite.texture;

	#ifdef SHADING_ENABLED
	static const byte min_weapon_shade = 70;
	byte shade = shade_at(settings.screen_height, pos);
	if (shade < min_weapon_shade) shade = min_weapon_shade;
	SDL_SetTextureColorMod(texture, shade, shade, shade);
	#else
	(void) pos;
	#endif

	// renders to shape buffer
	SDL_RenderCopy(screen.renderer, texture, &sheet_crop, &screen_pos);
	if (in_use) progress_animation_data_frame_ind(animation_data);
}
