/* Four enemy states: Idle, Chasing, Attacking, Dead
The spritesheet layout is in the order of the enemy states.
The sounds are in the same order, but with Attacked added after Dead. */

static const byte // sound chance at tick = numerator_sound_chancee / max_rand_sound_chance
	max_rand_sound_chance = 200, numerator_sound_chance = 1,
	dist_wake_from_sound = 5, attack_time_spacing = 1;

/* If the player's weapon has a y-pitch that is within the vertical bounds of the enemy, 
a flag is set in the enemy instance to allow them to be attacked by a weapon */
void update_enemy_weapon_y_state(EnemyInstance* const enemy_instance, const SDL_FRect* const thing_screen_pos) {
	const byte weapon_y_matches_enemy_y =
		settings.half_screen_height >= thing_screen_pos -> y &&
		thing_screen_pos -> y + thing_screen_pos -> h >= settings.half_screen_height;

	bit_to_x(enemy_instance -> status, mask_weapon_y_pitch_in_range_of_enemy, weapon_y_matches_enemy_y);
}

void set_enemy_instance_state(EnemyInstance* const enemy_instance, const EnemyState new_state,
	const byte silent, vec p_pos, const double p_height) {

	if (enemy_instance -> state == new_state) return;

	const Enemy* const enemy = enemy_instance -> enemy;

	enemy_instance -> state = new_state;
	if (!silent)
		play_sound_from_billboard_data(
			&enemy -> sounds[enemy_instance -> state],
			&enemy_instance -> billboard_data, p_pos, p_height);

	int new_frame_ind = 0;
	for (byte i = 0; i < enemy_instance -> state; i++)
		new_frame_ind += enemy -> animation_seg_lengths[i];

	enemy_instance -> mut_animation_data.frame_ind = new_frame_ind;
}

/*
The long-range enemy AI:
If the player meets the activation distance:
	While not reached target:
		Step closer
		Wait a bit
		Shoot
*/

void short_range_enemy_attack(const Enemy* const enemy, EnemyInstance* const enemy_instance, Player* const player,
	const double dist, const double height_diff) {

	const double curr_time = SDL_GetTicks() / 1000.0;
	if (curr_time - enemy_instance -> time_at_attack > attack_time_spacing && dist <= 1.0 && height_diff < 1.0) {
		enemy_instance -> time_at_attack = curr_time;

		// when the decr hp is less than zero, the enemy instance clips into walls - why?
		const double decr_hp = enemy -> power * (1.0 - dist * dist); // more damage closer

		if ((player -> hp -= decr_hp) <= 0.0) {
			player -> is_dead = 1;
			player -> hp = 0.0;
			for (byte i = 0; i < current_level.enemy_instance_count; i++)
				set_enemy_instance_state(enemy_instance, Idle, 1, player -> pos, player -> jump.height);
		}
		else play_sound(&player -> sound_when_attacked);
	}
}

/*
static EnemyState next_enemy_state(EnemyInstance* const enemy_instance, Player* const player, const Weapon* const weapon) {
	const Enemy* const enemy = enemy_instance -> enemy;
	const DataBillboard* const billboard_data = &enemy_instance -> billboard_data;

	const double
		dist = billboard_data -> dist,
		height_diff = fabs(player -> jump.height) - billboard_data -> height;

	const EnemyState prev_state = enemy_instance -> state;
	switch (prev_state) {
		case Idle: {
			if (height_diff >= 1.0) break;

			const byte awoke_from_sound = (dist <= dist_wake_from_sound) &&
				(bit_is_set(weapon -> status, mask_recently_used_weapon) || player -> jump.made_noise);

			if (awoke_from_sound || (dist <= enemy -> dist_wake_from_idle)
				|| bit_is_set(enemy_instance -> status, mask_recently_attacked_enemy))
				return Chasing;

			break;
		}

		case Chasing: { // pause and transition to attacking for long-range enemies
			const NavigationState nav_state = update_route_if_needed(&enemy_instance -> nav, player -> pos);
			if (nav_state == ReachedDest) return Attacking;
			else if (nav_state == PathTooLongBFS || nav_state == FailedBFS) return Idle;
			break;
		}

		case Attacking: {
			const NavigationState nav_state = update_route_if_needed(&enemy_instance -> nav, player -> pos);

			if (nav_state == Navigating) return Chasing;
			else if (nav_state == FailedBFS) return Idle;
			else if (bit_is_set(enemy_instance -> status, mask_long_range_attack_enemy)) {
				puts("Long range enemies are not supported yet");
				break;
			}

			short_range_enemy_attack(enemy, enemy_instance, player, dist, height_diff);
			break;
		}
		case Dead: break;
	}
	return prev_state;
}
*/

static void update_enemy_instance(EnemyInstance* const enemy_instance, Player* const player, const Weapon* const weapon) {
	if (enemy_instance -> state == Dead) return;

	const Enemy* const enemy = enemy_instance -> enemy;

	Navigator* const nav = &enemy_instance -> nav;
	DataBillboard* const billboard_data = &enemy_instance -> billboard_data;

	const vec p_pos = player -> pos;
	const double p_height = player -> jump.height;

	if (teleport_if_needed(&billboard_data -> pos, &billboard_data -> height, player, 0)) {
		set_enemy_instance_state(enemy_instance, Idle, 0, p_pos, p_height);
		return;
	}

	const double
		dist = billboard_data -> dist,
		height_diff = fabs(p_height - billboard_data -> height);

	const EnemyState last_state = enemy_instance -> state;

	/* for each state (excluding Dead), periodically play the sound from that state,
	and only play the Dead animation once, stopping on the last frame */
	switch (enemy_instance -> state) {
		case Idle: {
			if (height_diff >= 1.0) break;

			const byte awoke_from_sound = (dist <= dist_wake_from_sound) &&
				(bit_is_set(weapon -> status, mask_recently_used_weapon) || player -> jump.made_noise);

			if (awoke_from_sound || (dist <= enemy -> dist_wake_from_idle) || bit_is_set(enemy_instance -> status, mask_recently_attacked_enemy))
				set_enemy_instance_state(enemy_instance, Chasing, 0, p_pos, p_height);
		}
			break;

		case Chasing: { // pause and transition to attacking for long-range enemies
			const NavigationState nav_state = update_route_if_needed(nav, p_pos);
			if (nav_state == ReachedDest)
				set_enemy_instance_state(enemy_instance, Attacking, 0, p_pos, p_height);
			else if (nav_state == PathTooLongBFS || nav_state == FailedBFS)
				set_enemy_instance_state(enemy_instance, Idle, 0, p_pos, p_height);
		}
			break;

		case Attacking: {
			const NavigationState nav_state = update_route_if_needed(nav, p_pos);

			if (nav_state == Navigating)
				set_enemy_instance_state(enemy_instance, Chasing, 0, p_pos, p_height);
			else if (nav_state == FailedBFS)
				set_enemy_instance_state(enemy_instance, Idle, 0, p_pos, p_height);

			else if (bit_is_set(enemy_instance -> status, mask_long_range_attack_enemy))
				puts("Long range enemies are not supported yet");

			else short_range_enemy_attack(enemy, enemy_instance, player, dist, height_diff);
		}
			break;

		case Dead: break;
	}

	// sound happens at state change, so only one sound at once
	if (enemy_instance -> state == last_state) {
		const byte rand_num = (rand() % max_rand_sound_chance) + 1; // inclusive, 1 to max
		if (rand_num <= numerator_sound_chance) play_sound_from_billboard_data(&enemy -> sounds[enemy_instance -> state], &enemy_instance -> billboard_data, player -> pos, player -> jump.height);
	}

	clear_bit(enemy_instance -> status, mask_recently_attacked_enemy);
}

inlinable void update_all_enemy_instances(Player* const player, const Weapon* const weapon) {
	for (byte i = 0; i < current_level.enemy_instance_count; i++)
		update_enemy_instance(&current_level.enemy_instances[i], player, weapon);
}

void deinit_enemy(const Enemy* const enemy) {
	deinit_sprite(enemy -> animation_data.sprite);
	for (byte i = 0; i < 5; i++) deinit_sound(&enemy -> sounds[i]);
}
