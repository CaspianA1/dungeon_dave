typedef struct {
	byte enabled;
	Animation animation;
	int max_width, max_height;
} AnimatedSkybox;

void draw_animated_skybox(const AnimatedSkybox* as_ref, const double player_angle) {
	AnimatedSkybox as = *as_ref;

	const Animation* animation = &as_ref -> animation;

	/////

	const VectorI frame_origin = get_spritesheet_frame_origin(animation);

	const SDL_Rect sheet_crop = {
		frame_origin.x, frame_origin.y,
		animation -> frame_w, animation -> frame_h
	};

	/////

	const Sprite sprite = animation -> billboard.sprite;

	const double turn_percent = player_angle / 360.0;
	const int
		src_col_index = turn_percent * as.max_width,
		src_width = as.max_width / 4.0;

	if (turn_percent > 0.75) {
		const double err_amt = (turn_percent - 0.75) * 4.0;
		const int src_error = as.max_width * err_amt;
		const double dest_error = settings.screen_width * err_amt;

		const SDL_Rect
			src_1 = {src_col_index, 0, src_width, as.max_height},
			dest_1 = {0, 0, settings.screen_width - dest_error, settings.screen_height};
		SDL_RenderCopy(screen.renderer, sprite.texture, &src_1, &dest_1);

		const SDL_Rect
			src_2 = {0, 0, src_error / 4, as.max_height},
			dest_2 = {dest_1.w, 0, settings.screen_width - dest_1.w, settings.screen_height};
		SDL_RenderCopy(screen.renderer, sprite.texture, &src_2, &dest_2);
	}

	else {
		const SDL_Rect src = {src_col_index, 0, src_width, as.max_height};
		SDL_RenderCopy(screen.renderer, sprite.texture, &src, NULL);
	}
}

/////