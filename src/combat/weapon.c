void deinit_weapon(const Weapon weapon) {
	deinit_sound(weapon.sound);
	deinit_sprite(weapon.animation.billboard.sprite);
}

void shoot_weapon(const Weapon* const weapon, const vec pos, const vec dir) {
	DataDDA bullet = init_dda(pos, dir);

	while (iter_dda(&bullet)) {
		const ivec bullet_tile = bullet.curr_tile;
		if (map_point(current_level.wall_data, bullet_tile.x, bullet_tile.y)) break;

		for (byte i = 0; i < current_level.enemy_count; i++) {
			Enemy* const enemy = &current_level.enemies[i];
			if (enemy -> state == Dead) continue;

			const vec
				enemy_pos = enemy -> animations.billboard.pos,
				bullet_pos = vec_line_pos(pos, dir, bullet.dist);

			if (!vec_delta_exceeds(enemy_pos, bullet_pos, weapon -> dist_for_hit)) {
				enemy -> recently_attacked = 1;
				enemy -> hp -= weapon -> power;
				if (enemy -> hp <= 0.0) set_enemy_state(enemy, Dead, 0);
				else play_sound(enemy -> sounds[4], 0); // attacked
				return;
			}
		}
		if (weapon -> short_range) break;
	}
}

#ifndef NOCLIP_MODE

void use_weapon_if_needed(Weapon* const weapon, const Player player, const InputStatus input_status) {
	if (weapon -> in_use && weapon -> animation.frame_ind == 0) weapon -> in_use = 0;
	else if (input_status == BeginAnimatingWeapon && !weapon -> in_use) {
		weapon -> in_use = 1;
		play_sound(weapon -> sound, 0);
		shoot_weapon(weapon, player.pos, player.dir);
	}

	// -1 -> cycle frame, 0 -> first frame
	animate_weapon(&weapon -> animation, player.pos, weapon -> paces_sideways, weapon -> in_use,
		player.y_pitch, player.pace.screen_offset);
}

#else

#define use_weapon_if_needed(a, b, c)

#endif
