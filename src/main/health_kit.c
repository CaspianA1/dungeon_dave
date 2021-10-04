static Sprite health_kit_sprite;
static Sound health_kit_sound;
static const byte health_incr_amount = 5;

void init_health_kit_resources(void) {
	health_kit_sprite = init_sprite("assets/objects/health_kit.bmp", 0);
	health_kit_sound = init_sound("assets/audio/sound_effects/health_increase.wav", 1);
}

void deinit_health_kit_resources(void) {
	deinit_sprite(health_kit_sprite);
	deinit_sound(&health_kit_sound);
}

void use_health_kit_if_needed(Player* const player) {
	const vec teleporter_box_dimensions = vec_fill(actor_box_side_len);
	const BoundingBox player_box = init_bounding_box(player -> pos, vec_fill(actor_box_side_len));

	for (byte i = 0; i < current_level.health_kit_count; i++) {
		HealthKit* const health_kit = current_level.health_kits + i;
		if (!health_kit -> used
			&& aabb_collision(player_box, init_bounding_box(health_kit -> pos, teleporter_box_dimensions))) {

			player -> hp += health_incr_amount;
			if (player -> hp / INIT_HP > 1.0) player -> hp = INIT_HP;
			play_sound(&health_kit_sound);
			health_kit -> used = 1;
			break;
		}
	}
}
