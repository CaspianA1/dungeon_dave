/* Four enemy states: Idle, Chasing, Attacking, Dead
The spritesheet layout is in the order of the enemy states.
The sounds are in the same order, but with Attacked added after Dead. */

static const byte // sound chance at tick = numerator_sound_chancee / max_rand_sound_chance
	max_rand_sound_chance = 200, numerator_sound_chance = 1,
	dist_wake_from_sound = 5, attack_time_spacing = 1;

void set_enemy_instance_state(EnemyInstance* const enemy_instance, const EnemyState new_state, const byte silent) {
	if (enemy_instance -> state == new_state) return;

	const Enemy* const enemy = enemy_instance -> enemy;

	enemy_instance -> state = new_state;
	if (!silent) {play_sound(enemy -> sounds[enemy_instance -> state], 0);}

	int new_frame_ind = 0;
	for (byte i = 0; i < enemy_instance -> state; i++)
		new_frame_ind += enemy -> animation_seg_lengths[i];

	enemy_instance -> mut_animation_data.frame_ind = new_frame_ind;
}

void update_enemy_instance(EnemyInstance* const enemy_instance, Player* const player, const Weapon* const weapon) {
	if (enemy_instance -> state == Dead) return;

	const Enemy* const enemy = enemy_instance -> enemy;

	Navigator* const nav = &enemy_instance -> nav;
	double dist = enemy_instance -> billboard_data.dist;

	for (byte i = 0; i < 5; i++) set_sound_volume_from_dist(enemy -> sounds[i], dist);

	const EnemyState last_state = enemy_instance -> state;

	/* for each state (excluding Dead), periodically play the sound from that state,
	and only play the Dead animation once, stopping on the last frame */
	switch (enemy_instance -> state) {
		case Idle: {
			const byte awoke_from_sound = (dist <= dist_wake_from_sound) &&
				((weapon -> status & mask_recently_used) || player -> jump.made_noise);
				// if a player made a sound and they are close

			if (awoke_from_sound || (dist <= enemy -> dist_wake_from_idle) || enemy_instance -> recently_attacked)
				set_enemy_instance_state(enemy_instance, Chasing, 0);
		}
			break;
		
		case Chasing: {
			const NavigationState nav_state = update_path_if_needed(nav, player -> pos, player -> jump.height);
			if (nav_state == ReachedDest)
				set_enemy_instance_state(enemy_instance, Attacking, 0);
			else if (nav_state == PathTooLongBFS)
				set_enemy_instance_state(enemy_instance, Idle, 0);
		}
			break;

		case Attacking: {
			const NavigationState nav_state = update_path_if_needed(nav, player -> pos, player -> jump.height);

			if (nav_state == Navigating)
				set_enemy_instance_state(enemy_instance, Chasing, 0);
			else if (nav_state == FailedBFS)
				set_enemy_instance_state(enemy_instance, Idle, 0);

			else {
				const double curr_time = SDL_GetTicks() / 1000.0;
				if (curr_time - enemy_instance -> time_at_attack > attack_time_spacing && dist <= 1.0) {

					enemy_instance -> time_at_attack = curr_time;

					// when the decr hp is less than zero, the enemy instance clips into walls - why?
					const double decr_hp = enemy -> power * (1.0 - dist * dist); // more damage closer

					if ((player -> hp -= decr_hp) <= 0.0) {
						player -> is_dead = 1;
						player -> hp = 0.0;
						for (byte i = 0; i < current_level.enemy_instance_count; i++)
							set_enemy_instance_state(enemy_instance, Idle, 1);
					}
					else play_sound(player -> sound_when_attacked, 0);
				}
			}
		}
			break;

		case Dead: break;
	}

	 // sound happens at state change, so only one sound at once
	if (enemy_instance -> state == last_state) {
		const byte rand_num = (rand() % max_rand_sound_chance) + 1; // inclusive, 1 to max
		if (rand_num <= numerator_sound_chance) play_sound(enemy -> sounds[enemy_instance -> state], 0);
	}

	enemy_instance -> recently_attacked = 0;
}

inlinable void update_all_enemy_instances(Player* const player, const Weapon* const weapon) {
	for (byte i = 0; i < current_level.enemy_instance_count; i++)
		update_enemy_instance(&current_level.enemy_instances[i], player, weapon);
}

void deinit_enemy(const Enemy* const enemy) {
	deinit_sprite(enemy -> animation_data.sprite);
	for (byte i = 0; i < 5; i++) deinit_sound(enemy -> sounds[i]);
}
