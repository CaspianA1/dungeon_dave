void deinit_weapon(const Weapon* const weapon) {
	deinit_sound(&weapon -> sound);
	deinit_sprite(weapon -> animation_data.immut.sprite);
}

#ifndef NOCLIP_MODE

#ifdef DISABLE_ENEMIES
#define shoot_weapon(a, b, c, d)
#else

static void shoot_weapon(const Weapon* const weapon, const vec pos, const vec dir, const double p_height) {
	DataDDA bullet = init_dda((double[2]) UNPACK_2(pos), (double[2]) UNPACK_2(dir), 0.4);

	extern const double height_diff_for_interaction;
	while (iter_dda(&bullet)) {
		if (map_point(current_level.wall_data, bullet.curr_tile[0], bullet.curr_tile[1])) break;

		for (byte i = 0; i < current_level.enemy_instance_count; i++) {
			EnemyInstance* const enemy_instance = &current_level.enemy_instances[i];
			if (enemy_instance -> state == Dead) continue;

			const vec
				enemy_pos = enemy_instance -> billboard_data.pos,
				bullet_pos = vec_line_pos(pos, dir, bullet.dist);

			if (!vec_delta_exceeds(enemy_pos, bullet_pos, weapon -> dist_for_hit)
				&& fabs(enemy_instance -> billboard_data.height - p_height) <= height_diff_for_interaction) {

				enemy_instance -> recently_attacked = 1;
				enemy_instance -> hp -= weapon -> power;

				void set_enemy_instance_state(EnemyInstance* const, const EnemyState, const byte);
				if (enemy_instance -> hp <= 0.0) set_enemy_instance_state(enemy_instance, Dead, 0);
				else play_sound(&enemy_instance -> enemy -> sounds[4], 0); // attacked
				return;
			}
		}
		if (weapon -> status & mask_short_range) break;
	}
}

#endif

void use_weapon_if_needed(Weapon* const weapon, const Player* const player, const InputStatus input_status) {
	int* const frame_ind = &weapon -> animation_data.mut.frame_ind;

	if (player -> is_dead) *frame_ind = 0;

	const byte first_in_use = weapon -> status & mask_in_use;

	if (first_in_use && *frame_ind == 0)
		clear_nth_bit(&weapon -> status, 0); // not in use
	else if (input_status == BeginAnimatingWeapon && !first_in_use && !player -> is_dead) {
		set_nth_bit(&weapon -> status, 0); // in use
		set_nth_bit(&weapon -> status, 3); // recently used
		play_sound(&weapon -> sound, 0);
		shoot_weapon(weapon, player -> pos, player -> dir, player -> jump.height);
	}
	else clear_nth_bit(&weapon -> status, 3);

	// -1 -> cycle frame, 0 -> first frame
	animate_weapon(&weapon -> animation_data, player -> pos, weapon -> status & mask_paces_sideways,
		weapon -> status & mask_in_use, player -> y_pitch, player -> body.v);
}

#else

#define use_weapon_if_needed(a, b, c)

#endif
