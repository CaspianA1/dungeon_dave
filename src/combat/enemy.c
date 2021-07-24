/* Four enemy states: Idle, Chasing, Attacking, Dead
The spritesheet layout is in the order of the enemy states.
The sounds are in the same order, but with Attacked added after Dead. */

const int dist_wake_up_from_weapon = 5;
const double time_no_attacking_after_player_hit = 0.4;

void set_enemy_state(Enemy* const enemy, EnemyState new_state, byte silent) {
	if (enemy -> state == new_state) return;

	enemy -> state = new_state;
	if (!silent) {play_sound(enemy -> sounds[enemy -> state], 0);}

	int new_frame_ind = 0;
	for (byte i = 0; i < enemy -> state; i++)
		new_frame_ind += enemy -> animation_seg_lengths[i];

	enemy -> animations.frame_ind = new_frame_ind;
}

void update_enemy(Enemy* const enemy, Player* const player) {
	Navigator* const nav = &enemy -> nav;
	const double dist = enemy -> animations.billboard.dist;

	if (enemy -> recently_attacked) enemy -> time_at_attack = SDL_GetTicks() / 1000.0;

	/* for each state (excluding Dead), periodically play the sound from that state,
	and only play the Dead animation once, stopping on the last frame */
	switch (enemy -> state) {
		case Idle:
			if (dist <= enemy -> dist_wake_from_idle || enemy -> recently_attacked)
				set_enemy_state(enemy, Chasing, 0);
			break;
		
		case Chasing: {
			const NavigationState nav_state = update_path_if_needed(nav, player -> pos, player -> jump.height);
			if (nav_state == ReachedDest)
				set_enemy_state(enemy, Attacking, 0);
			else if (nav_state == PathTooLongBFS)
				set_enemy_state(enemy, Idle, 0);
		}
			break;

		case Attacking:
			if (update_path_if_needed(nav, player -> pos, player -> jump.height) == Navigating)
				set_enemy_state(enemy, Chasing, 0);
			else if ((SDL_GetTicks() / 1000.0 - enemy -> time_at_attack > time_no_attacking_after_player_hit)) {
				if ((player -> hp -= enemy -> power) <= 0.0) {
					player -> is_dead = 1;
					player -> hp = 0.0;
					for (byte i = 0; i < current_level.enemy_count; i++) set_enemy_state(enemy, Idle, 1);
				}
			}

			break;

		case Dead: break;
	}
	enemy -> recently_attacked = 0;
}

inlinable void update_all_enemies(Player* const player) {
	for (byte i = 0; i < current_level.enemy_count; i++)
		update_enemy(&current_level.enemies[i], player);
}

void deinit_enemy(const Enemy enemy) {
	deinit_sprite(enemy.animations.billboard.sprite);
	for (byte i = 0; i < 5; i++) deinit_sound(enemy.sounds[i]);
	wfree(enemy.sounds);
}
