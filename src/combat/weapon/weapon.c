void deinit_weapon(const Weapon* const weapon) {
	deinit_sound(&weapon -> sound);
	deinit_sprite(weapon -> animation_data.immut.sprite);
}

// Returns if damage was applied
byte apply_damage_from_weapon_if_needed(const Player* const player,
	const Weapon* const weapon, const float dist, const BoundingBox_3D projectile_box) {

	const vec p_pos = player -> pos;
	const double p_height = player -> jump.height;

	byte collided = 0;

	for (byte i = 0; i < current_level.enemy_instance_count; i++) {
		EnemyInstance* const enemy_instance = current_level.enemy_instances + i;
		if (enemy_instance -> state == Dead) continue;

		const DataBillboard* const billboard_data = &enemy_instance -> billboard_data;
		const BoundingBox_3D enemy_box = init_actor_bounding_box(billboard_data -> pos, billboard_data -> height);

		if (aabb_collision_3D(projectile_box, enemy_box)) {
			set_bit(enemy_instance -> flags, mask_recently_attacked_enemy);
			collided = 1;

			// `f(x) = 1 - (log2(x) / 8)`, range zero and below to infinity (smoothly decreasing slope)
			double percent_damage = 1.0 - (log2((double) dist) / 8.0);
			if (percent_damage > 1.0) percent_damage = 1.0;
			else if (percent_damage < 0.0) percent_damage = 0.0;

			void set_enemy_instance_state(EnemyInstance* const,
				const EnemyState, const byte, const vec, const double);

			if ((enemy_instance -> hp -= weapon -> power) <= 0.0)
				set_enemy_instance_state(enemy_instance, Dead, 0, p_pos, p_height);
			else {
				const int channel = play_short_sound(enemy_instance -> enemy -> sounds + 4); // attacked
				update_channel_from_thing_billboard_data(channel, &enemy_instance -> billboard_data, p_pos, p_height);
			}
		}
	}
	return collided;
}

void use_hitscan_weapon(const Weapon* const weapon, const Player* const player) {
	const byte short_range_weapon = bit_is_set(weapon -> flags, mask_short_range_weapon);

	Tracer tracer = init_tracer_from_player(player,
		short_range_weapon ? short_range_tracer_step : long_range_tracer_step, 1);

	while (iter_tracer(&tracer)) {
		const BoundingBox_3D projectile_box = init_bounding_box_3D(tracer.pos, hitscan_projectile_size);
		const byte damage_applied = apply_damage_from_weapon_if_needed(player, weapon, tracer.dist, projectile_box);
		if (damage_applied || short_range_weapon) break;
	}
}

#ifndef NOCLIP_MODE

void use_weapon_if_needed(Weapon* const weapon, const Player* const player, const InputStatus input_status) {
	int* const frame_ind = &weapon -> animation_data.mut.frame_ind;

	if (player -> is_dead) *frame_ind = 0;

	const byte
		first_in_use = bit_is_set(weapon -> flags, mask_in_use_weapon),
		spawns_projectile = bit_is_set(weapon -> flags, mask_spawns_projectile_weapon);

	void use_inter_tick_projectile_weapon(const Player* const, const int);
	void use_hitscan_weapon(const Weapon* const, const Player* const);

	if (first_in_use && *frame_ind == 0)
		clear_bit(weapon -> flags, mask_in_use_weapon);
	else if (input_status == BeginAnimatingWeapon && !first_in_use && !player -> is_dead) {
		set_bit(weapon -> flags, mask_in_use_weapon | mask_recently_used_weapon);
		const int channel = play_short_sound(&weapon -> sound);
		if (spawns_projectile) use_inter_tick_projectile_weapon(player, channel);
		else use_hitscan_weapon(weapon, player);
	}
	else clear_bit(weapon -> flags, mask_recently_used_weapon); // recently used = within the last tick
}

#else

#define use_weapon_if_needed(a, b, c)

#endif
