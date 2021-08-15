static Sprite teleporter_sprite;
static Sound teleporter_sound;

static const double teleporter_y_activation_delta = 1.0;
static const byte ticks_for_teleporter_fuzz = 50, num_fuzz_dots_on_screen = 80, dot_dimension_divisor = 15;
static const Color3 teleporter_color = {255, 204, 0};

void init_teleporter_data(void) {
	teleporter_sprite = init_sprite("assets/objects/teleporter.bmp", 0);
	teleporter_sound = init_sound("assets/audio/sound_effects/teleporter_zap.wav", 1);
}

void deinit_teleporter_data(void) {
	deinit_sprite(teleporter_sprite);
	deinit_sound(&teleporter_sound);
}

void teleport_if_needed(Player* const player) {
	static byte fuzz_ticks = ticks_for_teleporter_fuzz;

	if (fuzz_ticks < ticks_for_teleporter_fuzz) {
		if (fuzz_ticks-- == 0) fuzz_ticks = ticks_for_teleporter_fuzz;
		else {
			const double percent_ticks = 1.0 - (double) fuzz_ticks / ticks_for_teleporter_fuzz;
			SDL_SetRenderDrawColor(screen.renderer, teleporter_color.r * percent_ticks,
				teleporter_color.g * percent_ticks, teleporter_color.b * percent_ticks, SDL_ALPHA_OPAQUE);

			for (byte i = 0; i < num_fuzz_dots_on_screen; i++) {
				const byte dot_size = (rand() % (settings.avg_dimensions / dot_dimension_divisor)) * (1.0 - percent_ticks);

				const SDL_Rect dot_pos = {
					rand() % settings.screen_width, rand() % settings.screen_height,
					dot_size, dot_size
				};

				SDL_RenderFillRect(screen.renderer, &dot_pos);
			}
		}
	}

	for (byte i = 0; i < current_level.teleporter_count; i++) {
		const Teleporter teleporter = current_level.teleporters[i];

		if (!vec_delta_exceeds(player -> pos, teleporter.from_billboard.pos, thing_hit_dist) &&
			fabs(teleporter.from_billboard.height - player -> jump.height) <= teleporter_y_activation_delta) {

			fuzz_ticks--;

			play_sound(&teleporter_sound, 0);

			const vec dest = teleporter.to;
			const byte dest_point = map_point(current_level.wall_data, dest[0], dest[1]);
			player -> jump.height = current_level.get_point_height(dest_point, dest) + min_fall_height_for_sound;

			player -> pos = dest;
			break;
		}
	}
}
