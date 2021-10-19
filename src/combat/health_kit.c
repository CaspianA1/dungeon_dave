Sprite health_kit_sprite; // not static b/c used in things.c
static Sound health_kit_sound;
static const byte health_incr_amount = INIT_HP / 10;

void init_health_kit_resources(void) {
	health_kit_sprite = init_sprite("assets/objects/health_kit.bmp", D_Thing);
	health_kit_sound = init_sound("assets/audio/sound_effects/health_increase.wav", 1);
}

void deinit_health_kit_resources(void) {
	deinit_sprite(health_kit_sprite);
	deinit_sound(&health_kit_sound);
}

#ifndef PLANAR_MODE

void use_health_kit_if_needed(Player* const player) {
	const BoundingBox player_box = init_bounding_box(player -> pos, actor_box_side_len);

	for (byte i = 0; i < current_level.health_kit_count; i++) {
		HealthKit* const health_kit = current_level.health_kits + i;
		const DataBillboard* const billboard_data = &health_kit -> billboard;

		if (!health_kit -> used
			&& fabs(billboard_data -> height - player -> jump.height) < actor_height
			&& aabb_collision(player_box,
				init_bounding_box(billboard_data -> pos, actor_box_side_len))) {

			player -> hp += health_incr_amount;
			if (player -> hp > INIT_HP) player -> hp = INIT_HP;
			play_short_sound(&health_kit_sound);
			health_kit -> used = 1;
			break;
		}
	}
}
#else
#define use_health_kit_if_needed(a)
#endif
