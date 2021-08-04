void deinit_weapon(const Weapon weapon) {
	deinit_sound(weapon.sound);
	deinit_sprite(weapon.animation_data.sprite);
}

void shoot_weapon(const Weapon* const weapon, const vec pos, const vec dir) {
	DataDDA bullet = init_dda(pos, dir, 0.5);

	while (iter_dda(&bullet)) {
		const vec bullet_tile = {bullet.curr_tile[0], bullet.curr_tile[1]};
		if (map_point(current_level.wall_data, bullet_tile[0], bullet_tile[1])) break;

		for (byte i = 0; i < current_level.enemy_count; i++) {
			Enemy* const enemy = &current_level.enemies[i];
			if (enemy -> state == Dead) continue;

			const vec
				enemy_pos = enemy -> animated_billboard.billboard_data.pos,
				bullet_pos = vec_line_pos(pos, dir, bullet.dist);

			if (!vec_delta_exceeds(enemy_pos, bullet_pos, weapon -> dist_for_hit)) {
				enemy -> recently_attacked = 1;
				enemy -> hp -= weapon -> power;

				void set_enemy_state(Enemy* const, const EnemyState, const byte);
				if (enemy -> hp <= 0.0) set_enemy_state(enemy, Dead, 0);
				else play_sound(enemy -> sounds[4], 0); // attacked
				return;
			}
		}
		if (weapon -> status & mask_short_range) break;
	}
}

#ifndef NOCLIP_MODE

void use_weapon_if_needed(Weapon* const weapon, const Player* const player, const InputStatus input_status) {
	if (player -> is_dead) weapon -> animation_data.frame_ind = 0;

	const byte first_in_use = weapon -> status & mask_in_use;

	if (first_in_use && weapon -> animation_data.frame_ind == 0)
		clear_nth_bit(&weapon -> status, 0); // not in use
	else if (input_status == BeginAnimatingWeapon && !first_in_use && !player -> is_dead) {
		set_nth_bit(&weapon -> status, 0); // in use
		set_nth_bit(&weapon -> status, 3); // recently used
		play_sound(weapon -> sound, 0);
		shoot_weapon(weapon, player -> pos, player -> dir);
	}
	else clear_nth_bit(&weapon -> status, 3);

	// -1 -> cycle frame, 0 -> first frame
	animate_weapon(&weapon -> animation_data, player -> pos, weapon -> status & mask_paces_sideways,
		weapon -> status & mask_in_use, player -> y_pitch, player -> body.v);
}

#else

#define use_weapon_if_needed(a, b, c)

#endif
