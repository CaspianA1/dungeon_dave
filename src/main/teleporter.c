static Sprite teleporter_sprite;
static Sound teleporter_sound;

static const double teleporter_y_activation_delta = 1.0;

void init_teleporter_data(void) {
	teleporter_sprite = init_sprite("assets/objects/teleporter.bmp", 0);
	teleporter_sound = init_sound("assets/audio/sound_effects/teleporter_zap.wav", 1);
}

void deinit_teleporter_data(void) {
	deinit_sprite(teleporter_sprite);
	deinit_sound(&teleporter_sound);
}

void teleport_if_needed(Player* const player) {
	for (byte i = 0; i < current_level.teleporter_count; i++) {
		const Teleporter teleporter = current_level.teleporters[i];

		if (!vec_delta_exceeds(player -> pos, teleporter.from_billboard.pos, thing_hit_dist) &&
			fabs(teleporter.from_billboard.height - player -> jump.height) <= teleporter_y_activation_delta) {

			play_sound(&teleporter_sound, 0);

			const vec dest = teleporter.to;
			const byte dest_point = map_point(current_level.wall_data, dest[0], dest[1]);
			player -> jump.height = current_level.get_point_height(dest_point, dest) + min_fall_height_for_sound;

			player -> pos = dest;
			break;
		}
	}
}
