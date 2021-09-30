/* Four enemy states: Idle, Chasing, Attacking, Dead
The spritesheet layout is in the order of the enemy states.
The sounds are in the same order, but with Attacked added after Dead. */

static const byte // sound chance at tick = numerator_sound_chancee / max_rand_sound_chance
	max_rand_sound_chance = 200, numerator_sound_chance = 1, attack_time_spacing = 1;

/* If the player's weapon has a y-pitch that is within the vertical bounds of the enemy, 
a flag is set in the enemy instance to allow them to be attacked by a weapon */
void update_enemy_weapon_y_state(EnemyInstance* const enemy_instance, const SDL_FRect* const thing_screen_pos) {
	const byte weapon_y_matches_enemy_y =
		settings.half_screen_height >= thing_screen_pos -> y &&
		thing_screen_pos -> y + thing_screen_pos -> h >= settings.half_screen_height;

	bit_to_x(enemy_instance -> status, mask_weapon_y_pitch_in_range_of_enemy, weapon_y_matches_enemy_y);
}

void set_enemy_instance_state(EnemyInstance* const enemy_instance, const EnemyState new_state,
	const byte silent, const vec p_pos, const double p_height) {

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

void short_range_enemy_attack(const Enemy* const enemy,
	EnemyInstance* const enemy_instance, Player* const player, const double dist) {

	const double curr_time = SDL_GetTicks() / 1000.0;
	if (curr_time - enemy_instance -> time_at_attack > attack_time_spacing) {
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

static byte billboard_can_see_player(const DataBillboard* const billboard_data, const Player* const player) {
	const vec start_pos = billboard_data -> pos, possible_end_pos = player -> pos;

	const double src_height = billboard_data -> height;
	const ivec player_tile = ivec_from_vec(possible_end_pos);

	// direction = normalized delta vector between player and billboard
	DataDDA eye_ray = init_dda(start_pos, (possible_end_pos - start_pos) / vec_fill(billboard_data -> dist));

	do {
		const ivec curr_tile = eye_ray.curr_tile;

		if (*map_point(current_level.heightmap, curr_tile.x, curr_tile.y) > src_height)
			return 0;
		else if (curr_tile.x == player_tile.x && curr_tile.y == player_tile.y)
			return 1;

	} while (iter_dda(&eye_ray));

	return 0;
}

EnemyState next_enemy_state(EnemyInstance* const enemy_instance,
	Player* const player, const Weapon* const weapon) {

	const DataBillboard* const billboard_data = &enemy_instance -> billboard_data;
	const double dist = billboard_data -> dist;
	const vec p_pos = player -> pos, enemy_instance_pos = billboard_data -> pos;
	const Enemy* const enemy = enemy_instance -> enemy;

	const EnemyState prev_state = enemy_instance -> state;

	if (*map_point(current_level.heightmap, p_pos[0], p_pos[1])
		!= *map_point(current_level.heightmap, enemy_instance_pos[0], enemy_instance_pos[1]))
		return Idle;

	switch (prev_state) {
		case Idle: {
			const byte
				chase_from_sight = dist <= enemy -> dist_awaken.sight
					&& billboard_can_see_player(billboard_data, player),

				chase_from_sound = dist <= enemy -> dist_awaken.sound
					&& (player -> jump.made_noise || bit_is_set(weapon -> status, mask_recently_used_weapon));

			if (chase_from_sight || chase_from_sound || bit_is_set(enemy_instance -> status, mask_recently_attacked_enemy))
				return Chasing;

			break;
		}

		case Chasing:
			switch (update_route_if_needed(&enemy_instance -> nav, p_pos)) {
				case ReachedDest: return Attacking;
				case PathTooLongBFS: case FailedBFS: return Idle;
				default: break;
			}
			break;

		case Attacking:
			if (player_diverged_from_route_dest(&enemy_instance -> nav.route, p_pos))
				return Chasing;
			else if (bit_is_set(enemy_instance -> status, mask_long_range_attack_enemy))
				puts("Long range enemies are not supported yet");
			else short_range_enemy_attack(enemy, enemy_instance, player, dist);

			break;

		case Dead: break;
	}
	return prev_state;
}

static void new_update_enemy_instance(EnemyInstance* const enemy_instance,
	Player* const player, const Weapon* const weapon) {

	if (enemy_instance -> state == Dead) return;

	const vec p_pos = player -> pos;
	const double p_height = player -> jump.height;
	DataBillboard* const billboard_data = &enemy_instance -> billboard_data;

	if (teleport_if_needed(&billboard_data -> pos, &billboard_data -> height, player, 0)) {
		set_enemy_instance_state(enemy_instance, Idle, 0, p_pos, p_height);
		return;
	}

	const EnemyState
		prev_state = enemy_instance -> state,
		new_state = next_enemy_state(enemy_instance, player, weapon);

	set_enemy_instance_state(enemy_instance, new_state, 0, player -> pos, player -> jump.height);

	/* For each state (excluding Dead), this periodically play the sound
	from that state. Sounds only happen at state changes. */
	if (new_state == prev_state) {
		const byte rand_num = (rand() % max_rand_sound_chance) + 1; // inclusive, 1 to max
		if (rand_num <= numerator_sound_chance)
			play_sound_from_billboard_data(&enemy_instance -> enemy -> sounds[new_state],
				billboard_data, p_pos, p_height);
	}

	clear_bit(enemy_instance -> status, mask_recently_attacked_enemy);
}

inlinable void update_all_enemy_instances(Player* const player, const Weapon* const weapon) {
	for (byte i = 0; i < current_level.enemy_instance_count; i++)
		new_update_enemy_instance(&current_level.enemy_instances[i], player, weapon);
}

void deinit_enemy(const Enemy* const enemy) {
	deinit_sprite(enemy -> animation_data.sprite);
	for (byte i = 0; i < 5; i++) deinit_sound(&enemy -> sounds[i]);
}
