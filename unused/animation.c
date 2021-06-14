typedef struct {
	int frame_count, curr_frame, frame_drawn,
		per_row, per_col, frame_w, frame_h;
	float ms_btwn_frame;
	Uint32 last_frame_time;
	Sprite spritesheet;
} Animation;

Animation load_animation(const char* path, int fps, int frame_count, int per_row, int per_col) {
	Sprite spritesheet = load_sprite(path);
	Animation animation = {
		frame_count, 0, 0, per_row, per_col, spritesheet.surface -> w / per_row,
		spritesheet.surface -> h / per_col, SDL_GetTicks(), 1000.0 / fps * 1000.0,
		spritesheet
	};
	return animation;
};

void render_next_frame(
	Animation* animation, float dist_wall, float vline_x, float width_fov_ratio) {

	int row_ind = animation -> curr_frame / animation -> per_row; // y
	int col_ind = animation -> curr_frame - (row_ind * animation -> per_row); // x

	SDL_Rect cropped =
		{col_ind * animation -> frame_w, row_ind * animation -> frame_h,
		animation -> frame_w, animation -> frame_h};

	render_char_sprite(&animation -> spritesheet, &cropped, dist_wall, vline_x, width_fov_ratio);

	Uint32 curr_time = SDL_GetTicks();
	if (curr_time - animation -> last_frame_time >= animation -> ms_btwn_frame) {
		animation -> last_frame_time = curr_time;
		if (++animation -> curr_frame == animation -> frame_count)
			animation -> curr_frame = 0;
	}
}

extern inline void free_animation(Animation animation) {
	free_sprite(animation.spritesheet);
}