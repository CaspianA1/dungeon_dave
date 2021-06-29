/*
Four enemy states: Idle, Chasing, Attacking, Dead
The spritesheet layout is in the order of the enemy states.
The sounds are in the same order, but with Attacked added after Attacking.
*/

void shoot_enemy(Player player, Enemy* const enemy) {
	(void) player;
	(void) enemy;
	// const Billboard billboard = enemy -> animations.billboard;

	/* do DDA, if goes close enough to an enemy, shoot them. If a wall, stop, no hit.
	Checking beta doesn't help with walls. */

	/*
	if (fabs(billboard.beta) <= 0.3) {
		printf("Can shoot\n");
	}
	*/
}

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
	const EnemyDistThresholds thresholds = enemy -> dist_thresholds;

	// for each state (excluding Dead), periodically play the sound from that state
	switch (enemy -> state) {
		case Idle:
			if (dist <= thresholds.wake_from_idle) set_enemy_state(enemy, Chasing, 0);
			break;
		
		case Chasing: // done
			if (update_path_if_needed(nav, player.pos, player.jump) == ReachedDest)
				set_enemy_state(enemy, Attacking, 0);
			break;

		case Attacking: // done
			if (update_path_if_needed(nav, player.pos, player.jump) == Navigating)
				set_enemy_state(enemy, Chasing, 0);
			break;

		case Dead: break; // done
	}
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
	deinit_navigator(enemy.nav);	
}
