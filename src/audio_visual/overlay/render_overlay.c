// generic billboard = billboard || animation
int cmp_generic_billboards(const void* const a, const void* const b) {
	const double distances[2] = {
		((GenericBillboard*) a) -> billboard.billboard_data.dist,
		((GenericBillboard*) b) -> billboard.billboard_data.dist
	};

	if (distances[0] > distances[1]) return -1;
	else if (distances[0] < distances[1]) return 1;
	else return 0;
}

void draw_generic_billboards(const Player* const player, const double y_shift) {
	const double player_angle = to_radians(player -> angle);

	const byte generic_billboard_count = current_level.generic_billboard_count;
	GenericBillboard* const generic_billboards = current_level.generic_billboards;

	const byte start_of_enemies = current_level.billboard_count + current_level.animated_billboard_count;

	for (byte i = 0; i < generic_billboard_count; i++) {
		const byte
			is_animated = i >= current_level.billboard_count,
			is_enemy = i >= start_of_enemies,
			possible_animation_index = i - current_level.billboard_count,
			possible_enemy_index = i - start_of_enemies;

		Billboard billboard;
		DataBillboard* billboard_data = NULL;

		if (is_animated) {
			if (is_enemy) {
				AnimatedBillboard* const animated_billboard =
					&current_level.enemies[possible_enemy_index].animated_billboard;
				billboard = (Billboard) {
					animated_billboard -> animation_data.sprite, animated_billboard -> billboard_data
				};
				billboard_data = &animated_billboard -> billboard_data;
			}
			else {
				AnimatedBillboard* const animated_billboard =
					&current_level.animated_billboards[possible_animation_index];
				billboard = (Billboard) {
					animated_billboard -> animation_data.sprite, animated_billboard -> billboard_data
				};
				billboard_data = &animated_billboard -> billboard_data;
			}
		}
		else {
			billboard = current_level.billboards[i];
			billboard_data = &current_level.billboards[i].billboard_data;
		}

		const vec delta = billboard_data -> pos - player -> pos;
		billboard_data -> beta = atan2(delta[1], delta[0]) - player_angle;
		if (billboard_data -> beta < -two_pi) billboard_data -> beta += two_pi;
		billboard_data -> dist = sqrt(delta[0] * delta[0] + delta[1] * delta[1]);

		GenericBillboard* const generic_billboard = &generic_billboards[i];
		generic_billboard -> billboard = billboard;
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
		const DataBillboard billboard_data = billboard.billboard_data;

		const double
			abs_billboard_beta = fabs(billboard_data.beta),
			cos_billboard_beta = cos(billboard_data.beta);

		if (billboard_data.dist <= 0.01
			|| cos_billboard_beta <= 0.0
			|| doubles_eq(abs_billboard_beta, half_pi)
			|| doubles_eq(abs_billboard_beta, three_pi_over_two)
			|| doubles_eq(abs_billboard_beta, five_pi_over_two))
			continue;

		const double corrected_dist = billboard_data.dist * cos_billboard_beta;

		const double
			center_offset = tan(billboard_data.beta) * settings.proj_dist,
			size = settings.proj_dist / corrected_dist;

		const double
			center_x = settings.half_screen_width + center_offset,
			half_size = size / 2.0;

		const double start_x = center_x - half_size;
		if (start_x >= settings.screen_width) continue;

		double end_x = center_x + half_size;
		if (end_x < 0.0) continue;
		else if (end_x > settings.screen_width) end_x = settings.screen_width;

		//////////
		DataAnimation* possible_animation_data = NULL;
		SDL_Rect src_crop;
		int src_begin_x, width;

		if (generic.is_animated) {
			EnemyInstance* possible_enemy = NULL;

			if (generic.is_enemy) {
				possible_enemy = &current_level.enemies[generic.animation_index];
				possible_animation_data = &possible_enemy -> animated_billboard.animation_data;
			}
			else possible_animation_data = &current_level.animated_billboards[generic.animation_index].animation_data;

			const ivec frame_origin = get_spritesheet_frame_origin(possible_animation_data);

			src_begin_x = frame_origin.x;
			src_crop = (SDL_Rect) {
				.y = frame_origin.y,
				.w = 1, .h = possible_animation_data -> frame_h
			};

			if (generic.is_enemy) progress_enemy_frame_ind(possible_enemy);
			else progress_animation_data_frame_ind(possible_animation_data);

			width = possible_animation_data -> frame_w;
		}
		else {
			const ivec src_size = billboard.sprite.size;
			width = src_size.x;
			src_crop = (SDL_Rect) {.y = 0, .w = 1, .h = src_size.y};
			src_begin_x = 0;
		}
		//////////

		SDL_FRect screen_pos = {
			0.0, y_shift - half_size
			+ (player -> jump.height - billboard_data.height) * settings.screen_height / corrected_dist,
			settings.ray_column_width, size
		};

		#ifdef SHADING_ENABLED
		const byte shade = 255 * calculate_shade(size, billboard_data.pos);
		SDL_SetTextureColorMod(billboard.sprite.texture, shade, shade, shade);
		#endif

		for (int screen_row = start_x; screen_row < end_x; screen_row += settings.ray_column_width) {
			if (screen_row < 0 || (double) val_buffer[screen_row].depth < corrected_dist) continue;

			screen_pos.x = screen_row;
			const int src_offset = ((double) (screen_row - (int) start_x) / size) * width;
			src_crop.x = src_offset + src_begin_x;

			SDL_RenderCopyF(screen.renderer, billboard.sprite.texture, &src_crop, &screen_pos);
		}
	}
}

void draw_skybox(const double angle, const double y_shift) {
	const Skybox skybox = current_level.skybox;
	const ivec max_size = skybox.sprite.size;

	const double turn_percent = angle / 360.0;
	const int
		src_col_index = turn_percent * max_size.x,
		src_width = max_size.x / 4.0;

	const int dest_y = 0, dest_height = y_shift;
	const double look_up_percent = y_shift / settings.screen_height;

	const int src_height = max_size.y * look_up_percent;
	const int src_y = max_size.y - src_height;

	const SDL_Rect src_1 = {src_col_index, src_y, src_width, src_height};
	SDL_Rect dest_1 =  {0, dest_y, settings.screen_width, dest_height};

	if (turn_percent > 0.75) {
		const double err_amt = (turn_percent - 0.75) * 4.0;
		const int src_error = max_size.x * err_amt, dest_error = settings.screen_width * err_amt;

		dest_1.w -= dest_error;

		const SDL_Rect
			src_2 = {0, src_y, src_error / 4, src_height},
			dest_2 = {dest_1.w, dest_y, settings.screen_width - dest_1.w, dest_height};

		SDL_RenderCopy(screen.renderer, skybox.sprite.texture, &src_2, &dest_2);
	}
	SDL_RenderCopy(screen.renderer, skybox.sprite.texture, &src_1, &dest_1);
}
