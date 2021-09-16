void deinit_weapon(const Weapon* const weapon) {
	deinit_sound(&weapon -> sound);
	deinit_sprite(weapon -> animation_data.immut.sprite);
}

#ifndef NOCLIP_MODE

#ifdef DISABLE_ENEMIES
#define shoot_weapon(a, b, c, d)
#else

static void shoot_weapon(const Weapon* const weapon, const vec p_pos, const vec p_dir, const double p_height) {
	DataDDA bullet = init_dda(p_pos, p_dir, 0.4);

	while (iter_dda(&bullet)) {
		if (*map_point(current_level.wall_data, bullet.curr_tile[0], bullet.curr_tile[1])) break;

		for (byte i = 0; i < current_level.enemy_instance_count; i++) {
			EnemyInstance* const enemy_instance = &current_level.enemy_instances[i];
			if (enemy_instance -> state == Dead) continue;

			const vec
				enemy_pos = enemy_instance -> billboard_data.pos,
				bullet_pos = vec_line_pos(p_pos, p_dir, bullet.dist);

			if (!vec_delta_exceeds(enemy_pos, bullet_pos, weapon -> dist_for_hit)
				&& fabs(enemy_instance -> billboard_data.height - p_height) <= 1.0) {

				set_bit(enemy_instance -> status, mask_recently_attacked_enemy);
				enemy_instance -> hp -= weapon -> power;

				void set_enemy_instance_state(EnemyInstance* const, const EnemyState, const byte);
				if (enemy_instance -> hp <= 0.0) set_enemy_instance_state(enemy_instance, Dead, 0);
				else play_sound(&enemy_instance -> enemy -> sounds[4], 0); // attacked
				return;
			}
		}
		if (bit_is_set(weapon -> status, mask_short_range_weapon)) break;
	}
}

#endif

void use_weapon_if_needed(Weapon* const weapon, const Player* const player, const InputStatus input_status) {
	int* const frame_ind = &weapon -> animation_data.mut.frame_ind;

	if (player -> is_dead) *frame_ind = 0;

	const byte first_in_use = bit_is_set(weapon -> status, mask_in_use_weapon);

	if (first_in_use && *frame_ind == 0)
		clear_bit(weapon -> status, mask_in_use_weapon);
	else if (input_status == BeginAnimatingWeapon && !first_in_use && !player -> is_dead) {
		set_bit(weapon -> status, mask_in_use_weapon | mask_recently_used_weapon);
		play_sound(&weapon -> sound, 0);
		shoot_weapon(weapon, player -> pos, player -> dir, player -> jump.height);
	}
	else clear_bit(weapon -> status, mask_recently_used_weapon); // recently used = within the last tick

	// -1 -> cycle frame, 0 -> first frame
	animate_weapon(&weapon -> animation_data, player -> pos, bit_is_set(weapon -> status, mask_paces_sideways_weapon),
		bit_is_set(weapon -> status, mask_in_use_weapon), player -> body.v);
}

#else

#define use_weapon_if_needed(a, b, c)

#endif
