/*
Here is the naming system:
	- A Sprite is just something that is drawable, which can apply
		to Walls, Billboards, and Animations.

	- A Billboard is a Sprite that always faces the player. Their
		position can be changed theoretically as well.

	- An Animation is a Billboard that has a spritesheet as its Sprite field.
		It stores metadata about the animation, such as the frame index,
		and info regarding how to crop the spritesheet per frame.

I refer to a 'generic billboard' as a Billboard or Animation.
*/

int cmp_generic_billboards(const void* const a, const void* const b) {
	const VectorF distances = {
		((GenericBillboard*) a) -> billboard.dist,
		((GenericBillboard*) b) -> billboard.dist
	};

	if (distances[0] > distances[1]) return -1;
	else if (distances[0] < distances[1]) return 1;
	else return 0;
}

void draw_generic_billboards(const Player player, const double billboard_y_shift) {
	const double player_angle = to_radians(player.angle);

	const byte generic_billboard_count = current_level.generic_billboard_count;
	GenericBillboard* const generic_billboards = current_level.generic_billboards;

	const byte start_of_enemies = current_level.billboard_count + current_level.animation_count;

	for (byte i = 0; i < generic_billboard_count; i++) {
		const byte
			is_animated = i >= current_level.billboard_count,
			is_enemy = i >= start_of_enemies;

		const int
			possible_animation_index = i - current_level.billboard_count,
			possible_enemy_index = i - start_of_enemies;

		Billboard* billboard;

		if (is_animated)
			billboard = is_enemy
				? &current_level.enemies[possible_enemy_index].animations.billboard
				: &current_level.animations[possible_animation_index].billboard;
		else
			billboard = &current_level.billboards[i];

		const VectorF delta = VectorFF_sub(billboard -> pos, player.pos);
		billboard -> beta = atan2(delta[1], delta[0]) - player_angle;
		billboard -> dist = sqrt(delta[0] * delta[0] + delta[1] * delta[1]);

		GenericBillboard* const generic_billboard = &generic_billboards[i];
		generic_billboard -> billboard = *billboard;
		generic_billboard -> is_animated = is_animated;
		generic_billboard -> is_enemy = is_enemy;
		generic_billboard -> animation_index = is_enemy
			? possible_enemy_index
			: possible_animation_index;
	}	

	qsort(generic_billboards, generic_billboard_count, sizeof(GenericBillboard), cmp_generic_billboards);

	for (byte i = 0; i < generic_billboard_count; i++) {
		const GenericBillboard generic = generic_billboards[i];
		const Billboard billboard = generic.billboard;

		const double
			abs_billboard_beta = fabs(billboard.beta),
			cos_billboard_beta = cos(billboard.beta);

		if (billboard.dist <= 0.01
			|| cos_billboard_beta <= 0.0
			|| doubles_eq(abs_billboard_beta, half_pi, std_double_epsilon)
			|| doubles_eq(abs_billboard_beta, three_pi_over_two, std_double_epsilon)
			|| doubles_eq(abs_billboard_beta, five_pi_over_two, std_double_epsilon))
			continue;

		const double corrected_dist = billboard.dist * cos_billboard_beta;

		/* an experiment with not causing sprites to fly up
		when the projection distance increases

		static byte foo = 1;
		static double val;

		if (foo) {
			val = settings.proj_dist;
			foo = 0;
		}
		*/

		const double
			center_offset = tan(billboard.beta) * settings.proj_dist,
			size = settings.proj_dist / corrected_dist;

		const double
			center_x = settings.half_screen_width + center_offset,
			half_size = size / 2.0;

		const double start_x = center_x - half_size;
		if (start_x >= settings.screen_width) continue;

		double end_x = center_x + half_size;
		if (end_x < 0.0) continue;
		else if (end_x > settings.screen_width) end_x = settings.screen_width;

		/////

		Animation* possible_animation;
		SDL_Rect src_crop;
		int src_begin_x, width;

		if (generic.is_animated) {
			Enemy* possible_enemy;

			if (generic.is_enemy) {
				possible_enemy = &current_level.enemies[generic.animation_index];
				possible_animation = &possible_enemy -> animations;
			}
			else possible_animation = &current_level.animations[generic.animation_index];

			const VectorI frame_origin = get_spritesheet_frame_origin(*possible_animation);

			src_begin_x = frame_origin.x;
			src_crop = (SDL_Rect) {
				.y = frame_origin.y,
				.w = 1, .h = possible_animation -> frame_h
			};

			if (generic.is_enemy) progress_enemy_frame_ind(possible_enemy);
			else progress_animation_frame_ind(possible_animation);

			width = possible_animation -> frame_w;
		}
		else {
			const SDL_Surface* surface = billboard.sprite.surface;
			width = surface -> w;
			src_crop = (SDL_Rect) {.y = 0, .w = 1, .h = surface -> h};
			src_begin_x = 0;
		}

		/////

		SDL_FRect screen_pos = {
			0.0, billboard_y_shift - half_size
			+ (player.jump.height - billboard.height) * settings.screen_height / corrected_dist,
			settings.ray_column_width, size
		};

		const byte shade = 255 * calculate_shade(size, billboard.pos);
		SDL_SetTextureColorMod(billboard.sprite.texture, shade, shade, shade);

		for (int screen_row = start_x; screen_row < end_x; screen_row += settings.ray_column_width) {
			if (screen_row < 0 || screen.z_buffer[screen_row] < corrected_dist) continue;

			screen_pos.x = screen_row;
			const int src_offset = ((double) (screen_row - (int) start_x) / size) * width;
			src_crop.x = src_offset + src_begin_x;

			SDL_RenderCopyF(screen.renderer, billboard.sprite.texture, &src_crop, &screen_pos);
		}
	}
}

/*
https://zdoom.org/wiki/Free_look
https://zdoom.org/wiki/Sky
https://zdoom.org/wiki/Sky_stretching
*/

void draw_skybox(const double angle, const double y_shift) {
	const Skybox skybox = current_level.skybox;

	const double turn_percent = angle / 360.0;
	const int
		src_col_index = turn_percent * skybox.max_width,
		src_width = skybox.max_width / 4.0;

	const int dest_y = 0, dest_height = y_shift;
	double look_up_percent = y_shift / settings.screen_height;

	const int // src_height can be divided to determine what fraction is shown at once
		src_y = skybox.max_height * (1.0 - look_up_percent),
		src_height = skybox.max_height * look_up_percent;

	if (turn_percent > 0.75) {
		const double err_amt = (turn_percent - 0.75) * 4.0;
		const int src_error = skybox.max_width * err_amt;
		const double dest_error = settings.screen_width * err_amt;

		const SDL_Rect
			src_1 = {src_col_index, src_y, src_width, src_height},
			dest_1 = {0, dest_y, settings.screen_width - dest_error, dest_height};

		SDL_RenderCopy(screen.renderer, skybox.sprite.texture, &src_1, &dest_1);

		const SDL_Rect
			src_2 = {0, src_y, src_error / 4, src_height},
			dest_2 = {dest_1.w, dest_y, settings.screen_width - dest_1.w, dest_height};
		SDL_RenderCopy(screen.renderer, skybox.sprite.texture, &src_2, &dest_2);
	}

	else {
		const SDL_Rect
			src = {src_col_index, src_y, src_width, src_height},
			dest = {0, dest_y, settings.screen_width, dest_height};

		SDL_RenderCopy(screen.renderer, skybox.sprite.texture, &src, &dest);
	}
}
