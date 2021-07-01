/*
Four enemy states: Idle, Chasing, Attacking, Dead
The spritesheet layout is in the order of the enemy states.
The sounds are in the same order, but with Attacked added after Dead.
*/

void set_enemy_state(Enemy* const enemy, EnemyState new_state, byte silent) {
	enemy -> state = new_state;
	if (!silent) play_sound(enemy -> sounds[enemy -> state], 0); // stop the previous sound as well

	int new_frame_ind = 0;
	for (byte i = 0; i < enemy -> state; i++)
		new_frame_ind += enemy -> animation_seg_lengths[i];

	enemy -> animations.frame_ind = new_frame_ind;
}

void update_enemy(Enemy* const enemy, const Player player) {
	Navigator* const nav = &enemy -> nav;
	const double dist = fabs(*nav -> dist_to_player);

	/* for each state (excluding Dead), periodically play the sound from that state,
	and only play the Dead animation once, stopping on the last frame */
	switch (enemy -> state) {
		case Idle:
			if (dist <= enemy -> dist_wake_from_idle || enemy -> recently_attacked)
				set_enemy_state(enemy, Chasing, 0);
			break;
		
		case Chasing:
			if (dist >= enemy -> dist_return_to_idle)
				set_enemy_state(enemy, Idle, 0);
			else if (update_path_if_needed(nav, player.pos, player.jump) == ReachedDest)
				set_enemy_state(enemy, Attacking, 0);
			break;

		case Attacking:
			if (update_path_if_needed(nav, player.pos, player.jump) == Navigating)
				set_enemy_state(enemy, Chasing, 0);
			break;

		case Dead: break;
	}
	enemy -> recently_attacked = 0;
}

inlinable void update_all_enemies(const Player player) {
	for (byte i = 0; i < current_level.enemy_count; i++)
		update_enemy(&current_level.enemies[i], player);
}

void deinit_enemy(const Enemy enemy) {
	deinit_sprite(enemy.animations.billboard.sprite);
	for (byte i = 0; i < 5; i++)
		deinit_sound(enemy.sounds[i]);
	wfree(enemy.sounds);
	deinit_navigator(&enemy.nav);	
}
