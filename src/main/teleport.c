static Sprite teleporter_sprite;
static Sound teleporter_sound;

static const byte ticks_for_teleporter_fuzz = INIT_MAX_FPS, num_fuzz_dots_on_screen = 80, dot_dimension_divisor = 15;
static const Color3 teleporter_color = {255, 204, 0};

void init_teleporter_resources(void) {
	teleporter_sprite = init_sprite("assets/objects/teleporter.bmp", D_Thing);
	teleporter_sound = init_sound("assets/audio/sound_effects/teleporter_zap.wav", 1);
}

void deinit_teleporter_resources(void) {
	deinit_sprite(teleporter_sprite);
	deinit_sound(&teleporter_sound);
}

byte teleport_if_needed(vec* const pos, double* const height, const Player* const player) {
	const double teleporter_box_dimensions = actor_box_side_len + 0.2; // teleporter box is a bit bigger than the actor box
	const BoundingBox player_box = init_bounding_box(*pos, actor_box_side_len);

	for (byte i = 0; i < current_level.teleporter_count; i++) {
		const Teleporter teleporter = current_level.teleporters[i];

		if (aabb_collision(player_box, init_bounding_box(teleporter.from_billboard.pos, teleporter_box_dimensions))
			&& fabs(teleporter.from_billboard.height - *height) < actor_height) {

			/* if the player isn't teleporting, the pos and height data
			need to be passed twice through the player struct */
			const int channel = play_short_sound(&teleporter_sound);
			update_channel_from_thing_billboard_data(channel,
				&teleporter.from_billboard, player -> pos, player -> jump.height);

			const vec dest = teleporter.to;
			*height = *map_point(current_level.heightmap, dest[0], dest[1]);

			*pos = dest;
			return 1;
		}
	}
	return 0;
}

void teleport_player_if_needed(Player* const player) {
	static byte fuzz_ticks = ticks_for_teleporter_fuzz;

	if (fuzz_ticks < ticks_for_teleporter_fuzz) {
		if (fuzz_ticks-- == 0) fuzz_ticks = ticks_for_teleporter_fuzz;
		else {
			const double percent_ticks = 1.0 - (double) fuzz_ticks / ticks_for_teleporter_fuzz;
			SDL_SetRenderDrawColor(screen.renderer, teleporter_color.r * percent_ticks,
				teleporter_color.g * percent_ticks, teleporter_color.b * percent_ticks, SDL_ALPHA_OPAQUE);

			for (byte i = 0; i < num_fuzz_dots_on_screen; i++) {
				const byte dot_size =
					(rand() % (settings.avg_dimensions / dot_dimension_divisor)) * (1.0 - percent_ticks);

				const SDL_Rect dot_pos = {
					rand() % settings.screen_width, rand() % settings.screen_height,
					dot_size, dot_size
				};

				SDL_RenderFillRect(screen.renderer, &dot_pos);
			}
		}
	}

	const double prev_height = player -> jump.height;

	if (teleport_if_needed(&player -> pos, &player -> jump.height, player)) {
		/* If the player position curve is on its downward sloper, jumping is disabled
		because otherwise, the player may clip into the floor and fall into oblivion */
		if (prev_height < player -> jump.start_height) clear_bit(player -> jump.flags, mask_currently_jumping);
		else player -> jump.start_height = player -> jump.height;

		fuzz_ticks--;
	}
}
