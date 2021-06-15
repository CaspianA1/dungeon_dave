/*
Five enemy states: Idle, Chasing, Attacking, Retreating, Dead
The spritesheet layout is in the order of the enemy states.
The sounds are in the same order, but with Retreating substituted for Attacked.
*/

void set_enemy_state(Enemy* enemy, EnemyState new_state) {
	enemy -> state = new_state;

	int new_frame_ind = 0;
	for (byte i = 0; i < enemy -> state; i++)
		new_frame_ind += enemy -> animation_seg_lengths[i];

	enemy -> animations.frame_ind = new_frame_ind;
}

void retreat_enemy(Enemy* enemy, const Player player) {
	if (enemy -> state == Retreating) {
		update_path_if_needed(&enemy -> navigator, player.pos, player.jump);
	}

	else {
		while (1) {
			const VectorF new_spot = {rand() % current_level.map_width, rand() % current_level.map_height};
			if (wall_point(new_spot[0], new_spot[1])) continue;
			enemy -> animations.billboard.pos = new_spot;
			// if the new dest isn't navigatable to, find a new spot
			if (update_path_if_needed(&enemy -> navigator, player.pos, player.jump) == CouldNotNavigate)
				continue;

			set_enemy_state(enemy, Retreating);
			break;
		}
	}
}

void update_enemy(Enemy* enemy, const Player player) {
	static byte i = 1;
	if (i) play_sound(enemy -> sounds[enemy -> state], 0);
	i = 0;

	switch (enemy -> state) {
		case Idle: // stay there, idle animation
			break;
		case Chasing: { // bfs stuff
			if (update_path_if_needed(&enemy -> navigator, player.pos, player.jump) == ReachedDest)
				set_enemy_state(enemy, Attacking);
			break;
		}
		case Attacking: // attack animation + fighting
			// attack if the player is close enough, otherwise transition to Chasing
			break;
		case Retreating: // run away to some random vantage point
			break;
		case Dead: // do not move, dead single-sprite animation
			break;
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
	deinit_navigator(enemy.navigator);	
}
