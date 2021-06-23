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

		if (cos_billboard_beta <= 0.0
			|| doubles_eq(abs_billboard_beta, half_pi, std_double_epsilon)
			|| doubles_eq(abs_billboard_beta, three_pi_over_two, std_double_epsilon)
			|| doubles_eq(abs_billboard_beta, five_pi_over_two, std_double_epsilon))
			continue;

		const double corrected_dist = billboard.dist * cos_billboard_beta;

		const double
			center_offset = tan(billboard.beta) * settings.proj_dist,
			size = settings.proj_dist / corrected_dist;

		/////

		Animation* possible_animation;
		SDL_Rect possible_spritesheet_crop;
		double possible_spritesheet_begin_x, width;

		if (generic.is_animated) {
			Enemy* possible_enemy;

			if (generic.is_enemy) {
				possible_enemy = &current_level.enemies[generic.animation_index];
				possible_animation = &possible_enemy -> animations;
			}
			else
				possible_animation = &current_level.animations[generic.animation_index];

			const VectorI frame_origin = get_spritesheet_frame_origin(*possible_animation);

			possible_spritesheet_begin_x = frame_origin.x;
			possible_spritesheet_crop = (SDL_Rect) {
				.y = frame_origin.y,
				.w = 1, .h = possible_animation -> frame_h
			};

			if (generic.is_enemy) progress_enemy_frame_ind(possible_enemy);
			else progress_animation_frame_ind(possible_animation);

			width = possible_animation -> frame_w;
		}
		else width = billboard.sprite.surface -> w;

		/////

		const double
			center_x = settings.half_screen_width + center_offset,
			half_size = size / 2.0;

		const double start_x = center_x - half_size;
		if (start_x >= settings.screen_width) continue;

		double end_x = center_x + half_size;
		if (end_x < 0.0) continue;
		else if (end_x >= settings.screen_width) end_x = settings.screen_width - 1.0;

		SDL_FRect screen_pos = {
			0.0, billboard_y_shift - half_size
			+ (player.jump.height - billboard.height) * settings.screen_height / corrected_dist,
			settings.ray_column_width, size
		};

		for (int screen_row = start_x; screen_row < end_x; screen_row++) {
			if (screen_row < 0 || screen.z_buffer[screen_row] < corrected_dist) continue;

			const double offset = ((double) (screen_row - start_x) / size) * width;
			screen_pos.x = screen_row;

			if (generic.is_animated) {
				const byte shade = 255 * calculate_shade((double) screen_pos.h, billboard.pos);

				SDL_SetTextureColorMod(billboard.sprite.texture, shade, shade, shade);

				possible_spritesheet_crop.x = possible_spritesheet_begin_x + offset;

				SDL_RenderCopyF(screen.renderer, billboard.sprite.texture,
					&possible_spritesheet_crop, &screen_pos);
			}

			else draw_column(billboard.sprite, billboard.pos, offset, -1, -1, &screen_pos);
		}
	}
}

void draw_skybox(const double angle, const double y_shift) {
	const Skybox skybox = current_level.skybox;

	const double turn_percent = angle / 360.0;
	const int
		src_col_index = turn_percent * skybox.max_width,
		src_width = skybox.max_width / 4.0;

	//////////

	/*
	I need to adjust src_height and dest_height
	find out what percentage of the skybox I need to show when looking completely up
	also, the texture box shouldn't move down when looking up and down

	All of the dest math is correct

	some warping when looking up
	make the scroll rate even when looking up and down
	https://www.dcode.fr/function-equation-finder
	make sure that there is an equal amount of skybox to show when looking up and down
	*/

	const double two_thirds = 2.0 / 3.0;
	const double dest_y = 0, dest_height = y_shift; // maybe dest_y shouldn't always be 0
	const double y_shift_percentage = y_shift / settings.screen_height;
	const double show_percentage = y_shift_percentage * two_thirds;
	// DEBUG(dest_height, lf);

	int src_height = show_percentage * skybox.max_height;

	double test = two_thirds * y_shift_percentage * y_shift_percentage
				- 5.0 / 3.0 * y_shift_percentage + 1.0;

	int src_y = test * skybox.max_height;

	// printf("src_y = %d, src_height = %d\n", src_y, src_height);
	// DEBUG(src_y + src_height, d);

	// at the top: y = 0/3, height = 2/3
	// in the middle: y = 1/3, height = 1/3
	// at the bottom: y = 3/3, height = 0/3

	//////////

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
