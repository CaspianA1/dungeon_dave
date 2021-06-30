inlinable Weapon init_weapon(const char* const sound_path,
	const char* const spritesheet_path,
	const double screen_y_shift_percent_down, const double power, const int frames_per_row,
	const int frames_per_col, const int frame_count, const int fps) {

	return (Weapon) {0, screen_y_shift_percent_down, power,
		init_sound(sound_path, 1), init_animation(spritesheet_path, frames_per_row, frames_per_col, frame_count, fps)};
}

void deinit_weapon(const Weapon weapon) {
	deinit_sound(weapon.sound);
	deinit_sprite(weapon.animation.billboard.sprite);
}

void shoot_weapon(const Weapon* const weapon, const VectorF pos, const VectorF dir) {
	DataDDA bullet = init_dda(pos, dir);
	const double dist_squared_for_hit = 1.2;

	while (iter_dda(&bullet)) {
		const VectorI bullet_pos = bullet.curr_tile;
		if (map_point(current_level.wall_data, bullet_pos.x, bullet_pos.y)) break;

		for (byte i = 0; i < current_level.enemy_count; i++) {
			Enemy* const enemy = &current_level.enemies[i];
			if (enemy -> state == Dead) continue;

			const Billboard billboard = enemy -> animations.billboard;
			const VectorF delta = {fabs(bullet_pos.x - billboard.pos[0]), fabs(bullet_pos.y - billboard.pos[1])};
			const double bullet_dist_squared = delta[0] * delta[0] + delta[1] * delta[1];

			if (bullet_dist_squared <= dist_squared_for_hit) {
				enemy -> recently_attacked = 1;
				enemy -> hp -= weapon -> power;
				if (enemy -> hp <= 0.0) set_enemy_state(enemy, Dead, 0);
				else play_sound(enemy -> sounds[4], 0);
				goto finished_shooting;
			}
		}
	}
	finished_shooting:;
}

void use_weapon_if_needed(Weapon* const weapon, const Player player, const InputStatus input_status) {
	#ifndef NOCLIP_MODE

	if (weapon -> in_use && weapon -> animation.frame_ind == 0) weapon -> in_use = 0;
	else if (input_status == BeginAnimatingWeapon && !weapon -> in_use) {
		weapon -> in_use = 1;
		play_sound(weapon -> sound, 0);
		shoot_weapon(weapon, player.pos, player.dir);
	}

	// -1 -> cycle frame, 0 -> first frame
	animate_weapon(&weapon -> animation, player.pos, -weapon -> in_use,
		player.z_pitch, player.pace.screen_offset, weapon -> screen_y_shift_percent_down);

	#else

	(void) weapon;
	(void) player;
	(void) input_status;

	#endif
}
