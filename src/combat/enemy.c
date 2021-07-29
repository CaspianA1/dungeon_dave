/* Four enemy states: Idle, Chasing, Attacking, Dead
The spritesheet layout is in the order of the enemy states.
The sounds are in the same order, but with Attacked added after Dead. */

const int dist_wake_up_from_weapon = 5, attack_time_spacing = 1;

void set_enemy_state(Enemy* const enemy, EnemyState new_state, byte silent) {
	if (enemy -> state == new_state) return;

	enemy -> state = new_state;
	if (!silent) {play_sound(enemy -> sounds[enemy -> state], 0);}

	int new_frame_ind = 0;
	for (byte i = 0; i < enemy -> state; i++)
		new_frame_ind += enemy -> animation_seg_lengths[i];

	enemy -> animated_billboard.animation_data.frame_ind = new_frame_ind;
}

void update_enemy(Enemy* const enemy, Player* const player) {
	Navigator* const nav = &enemy -> nav;
	const double dist = enemy -> animated_billboard.billboard_data.dist;

	// if (enemy -> recently_attacked) enemy -> time_at_attack = SDL_GetTicks() / 1000.0;

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
			else {
				const double curr_time = SDL_GetTicks() / 1000.0;
				if (curr_time - enemy -> time_at_attack > attack_time_spacing) {
					enemy -> time_at_attack = curr_time;

					double dist = enemy -> animated_billboard.billboard_data.dist; // don't reuse dist
					if (dist > 1.0) dist = 1.0; // when the decr hp is less than zero, the enemy clips into walls - why?
					const double decr_hp = enemy -> power * (1.0 - dist * dist); // more damage closer

					play_sound(player -> sound_when_attacked, 0);

					if ((player -> hp -= decr_hp) <= 0.0) {
						player -> is_dead = 1;
						player -> hp = 0.0;
						for (byte i = 0; i < current_level.enemy_count; i++) set_enemy_state(enemy, Idle, 1);
					}
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

void deinit_enemy(const Enemy* const enemy) {
	deinit_sprite(enemy -> animated_billboard.animation_data.sprite);
	for (byte i = 0; i < 5; i++) deinit_sound(enemy -> sounds[i]);
	wfree(enemy -> sounds);
}
