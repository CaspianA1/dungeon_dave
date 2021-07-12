Sprite init_sprite(const char* const path) { // put in other file
	SDL_Surface* const surface = SDL_LoadBMP(path);
	if (surface == NULL) FAIL("Could not load a surface with the path of %s\n", path);

	const Sprite sprite = {SDL_CreateTextureFromSurface(screen.renderer, surface), {surface -> w, surface -> h}};

	/*
	const SDL_Texture* const prev_render_target = SDL_GetRenderTarget(screen.renderer);
	SDL_SetRenderTarget(screen.renderer, NULL);
	SDL_SetRenderTarget(screen.renderer, prev_render_target);
	*/

	SDL_FreeSurface(surface);
	return sprite;
}

inlinable void deinit_sprite(const Sprite sprite) {
	SDL_DestroyTexture(sprite.texture);
}

/////

inlinable Animation init_animation(const char* const path, const int frames_per_row,
	const int frames_per_col, const int frame_count, const int fps) {

	const Billboard billboard = {init_sprite(path), {0, 0}, 0, 0, 0};
	const ivec size = billboard.sprite.size;

	return (Animation) {
		billboard, frames_per_row, frames_per_col,
		size.x / frames_per_row,
		size.y / frames_per_col,
		frame_count, 0, 1.0 / fps, 0
	};
}

inlinable void progress_frame_ind(Animation* const animation, const int begin, const int end) {
	const double current_time = SDL_GetTicks() / 1000.0;
	const double time_delta = current_time - animation -> last_frame_time;

	if (time_delta >= animation -> secs_per_frame) {
		if (++animation -> frame_ind == end)
			animation -> frame_ind = begin;

		animation -> last_frame_time = current_time;
	}
}

inlinable void progress_animation_frame_ind(Animation* const animation) {
	progress_frame_ind(animation, 0, animation -> frame_count);
}

inlinable void progress_enemy_frame_ind(Enemy* const enemy) {
	int begin = 0;
	for (byte i = 0; i < enemy -> state; i++)
		begin += enemy -> animation_seg_lengths[i];

	const int end = begin + enemy -> animation_seg_lengths[enemy -> state];

	if (enemy -> state == Dead && enemy -> animations.frame_ind == end - 1) return;

	progress_frame_ind(&enemy -> animations, begin, end);
}

inlinable ivec get_spritesheet_frame_origin(const Animation animation) {
	const int y_ind = animation.frame_ind / animation.frames_per_row;
	const int x_ind = animation.frame_ind - y_ind * animation.frames_per_row;
	const ivec size = animation.billboard.sprite.size;

	return (ivec) {
		(double) x_ind / animation.frames_per_row * size.x,
		(double) y_ind / animation.frames_per_col * size.y
	};
}

void animate_weapon(Animation* const animation, const vec pos,
	const int frame_num, const int y_pitch, const double pace) {
 
	// frame_num == -1 -> auto_progress frame

	#ifndef SHADING_ENABLED
	(void) pos;
	#endif

	const ivec frame_origin = get_spritesheet_frame_origin(*animation);

	const SDL_Rect sheet_crop = {
		frame_origin.x, frame_origin.y,
		animation -> frame_w, animation -> frame_h
	};

	const SDL_FRect screen_pos = {
		pace,
		fabs(pace) + (y_pitch < 0 ? 0 : y_pitch),
		settings.screen_width,
		settings.screen_height
	};

	SDL_Texture* const texture = animation -> billboard.sprite.texture;

	byte shade = 255 * calculate_shade(settings.screen_height, pos);
	if (shade < 70) shade = 70;
	SDL_SetTextureColorMod(texture, shade, shade, shade);

	// renders to shape buffer
	SDL_RenderCopyF(screen.renderer, texture, &sheet_crop, &screen_pos);
	if (frame_num == -1) progress_animation_frame_ind(animation);
}
