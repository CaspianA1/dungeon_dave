/* Four enemy states: Idle, Chasing, Attacking, Dead
The spritesheet layout is in the order of the enemy states.
The sounds are in the same order, but with Attacked added after Dead. */

static const byte // sound chance at tick = numerator_sound_chancee / max_rand_sound_chance
	max_rand_sound_chance = 200,
	numerator_sound_chance = 1,
	attack_time_spacing_secs = 1;

void set_enemy_instance_state(EnemyInstance* const enemy_instance, const EnemyState new_state,
	const byte silent, const vec p_pos, const double p_height) {

	if (enemy_instance -> state == new_state) return;

	const Enemy* const enemy = enemy_instance -> enemy;

	enemy_instance -> state = new_state;
	if (!silent)
		play_sound_from_billboard_data(
			enemy -> sounds + new_state,
			&enemy_instance -> billboard_data, p_pos, p_height);

	int new_frame_ind = 0;
	for (byte i = 0; i < enemy_instance -> state; i++)
		new_frame_ind += enemy -> animation_seg_lengths[i];

	enemy_instance -> mut_animation_data.frame_ind = new_frame_ind;
}

inlinable void short_range_enemy_attack(const Enemy* const enemy,
	EnemyInstance* const enemy_instance, Player* const player, const double dist) {

	const double curr_time = SDL_GetTicks() / 1000.0;

	if (curr_time - enemy_instance -> time_at_attack > attack_time_spacing_secs) {
		enemy_instance -> time_at_attack = curr_time;

		/* The reason why this test isn't in the if statement above is because
		if it were, the enemy would try to attack again at the next tick,
		making it very hard to jump over them to avoid their attack; so this
		essentially gives enemies a recharge time for their next attack. */
		if (fabs(enemy_instance -> billboard_data.height - player -> jump.height) >= actor_height) return;

		/* dist is guaranteed to be less than 1, according to the code in
		update_route_if_neede; so decr_hp will never be negative */
		const double decr_hp = enemy -> power * (enemy_dist_for_attack - dist * dist); // more damage closer

		if ((player -> hp -= decr_hp) <= 0.0) {
			player -> is_dead = 1;
			player -> hp = 0.0;
			for (byte i = 0; i < current_level.enemy_instance_count; i++)
				set_enemy_instance_state(enemy_instance, Idle, 1, player -> pos, player -> jump.height);
		}
		else play_sound(&player -> sound_when_attacked);
	}
}

inlinable void long_range_enemy_attack(const Enemy* const enemy,
	EnemyInstance* const enemy_instance, Player* const player, const double dist) {

	(void) enemy;
	(void) enemy_instance;
	(void) player;
	(void) dist;

	puts("Long range enemies are not supported yet");
}

static byte billboard_can_see_player(const DataBillboard* const billboard_data, const Player* const player) {
	static const double eye_trace_step = 0.1, eye_box_size = 0.2;

	const vec p_pos = player -> pos;
	const double p_height = player -> jump.height;

	const double dist_diff = billboard_data -> dist; // TODO: to one over
	const double height_diff = fabs(billboard_data -> height - p_height);
	const vec dir_2D = (p_pos - billboard_data -> pos) / vec_fill(dist_diff);

	Hitscan hitscan = {
		{billboard_data -> pos[0], billboard_data -> pos[1], billboard_data -> height + actor_eye_height},
		{dir_2D[0], dir_2D[1], atan(height_diff / dist_diff)}, 0.0, eye_trace_step
	};

	const BoundingBox_3D player_box = init_actor_bounding_box(p_pos, p_height);

	while (iter_hitscan(&hitscan)) {
		const BoundingBox_3D eye_box = init_bounding_box_3D(hitscan.pos, eye_box_size);
		if (aabb_collision_3D(player_box, eye_box)) return 1;
		else if (point_exists_at((double) hitscan.pos[0], (double) hitscan.pos[1], (double) hitscan.pos[2])) return 0;
	}

	return 0;
}

static EnemyState next_enemy_state(EnemyInstance* const enemy_instance,
	Player* const player, const Weapon* const weapon) {

	const DataBillboard* const billboard_data = &enemy_instance -> billboard_data;
	const double dist = billboard_data -> dist;
	const vec p_pos = player -> pos, enemy_instance_pos = billboard_data -> pos;
	const Enemy* const enemy = enemy_instance -> enemy;

	const EnemyState prev_state = enemy_instance -> state;

	const byte
		base_heights_not_eq =
			*map_point(current_level.heightmap, p_pos[0], p_pos[1])
			!= *map_point(current_level.heightmap, enemy_instance_pos[0], enemy_instance_pos[1]),

		long_range_attacker = bit_is_set(enemy_instance -> flags, mask_long_range_attack_enemy);

	switch (prev_state) {
		case Idle: {
			const byte
				chase_from_sight = dist <= enemy -> dist_awaken.sight
					&& billboard_can_see_player(billboard_data, player),

				chase_from_sound = dist <= enemy -> dist_awaken.sound
					&& (bit_is_set(player -> jump.flags, mask_made_noise_jump)
						|| bit_is_set(weapon -> flags, mask_recently_used_weapon));

			if (!long_range_attacker && base_heights_not_eq) break;

			else if (chase_from_sight || chase_from_sound || bit_is_set(enemy_instance -> flags, mask_recently_attacked_enemy))
				return Chasing;
			break;
		}

		case Chasing:
			if (base_heights_not_eq && long_range_attacker) return Attacking;

			switch (update_route_if_needed(&enemy_instance -> nav, p_pos, billboard_data -> height)) {
				case ReachedDest: return Attacking;
				case PathTooLongBFS: case FailedBFS: return Idle;
				default: break;
			}
			break;

		case Attacking:
			if (player_diverged_from_route_dest(&enemy_instance -> nav.route, p_pos) && !base_heights_not_eq)
				return Chasing;
			else
				if (!billboard_can_see_player(billboard_data, player))
					return Idle;

				(long_range_attacker ? long_range_enemy_attack : short_range_enemy_attack)
					(enemy, enemy_instance, player, dist);

			break;

		case Dead: break;
	}
	return prev_state;
}

static void update_enemy_instance(EnemyInstance* const enemy_instance,
	Player* const player, const Weapon* const weapon) {

	if (enemy_instance -> state == Dead) return;

	const vec p_pos = player -> pos;
	const double p_height = player -> jump.height;
	DataBillboard* const billboard_data = &enemy_instance -> billboard_data;

	if (teleport_if_needed(&billboard_data -> pos, &billboard_data -> height, player)) {
		set_enemy_instance_state(enemy_instance, Idle, 0, p_pos, p_height);
		return;
	}

	const EnemyState
		prev_state = enemy_instance -> state,
		new_state = next_enemy_state(enemy_instance, player, weapon);

	set_enemy_instance_state(enemy_instance, new_state, 0, p_pos, p_height);

	/* For each state (excluding Dead), this periodically play the sound
	from that state. Sounds only happen at state changes. */
	if (new_state == prev_state) {
		const byte rand_num = (rand() % max_rand_sound_chance) + 1; // inclusive, 1 to max
		if (rand_num <= numerator_sound_chance)
			play_sound_from_billboard_data(&enemy_instance -> enemy -> sounds[new_state],
				billboard_data, p_pos, p_height);
	}

	clear_bit(enemy_instance -> flags, mask_recently_attacked_enemy);
}

inlinable void update_all_enemy_instances(Player* const player, const Weapon* const weapon) {
	for (byte i = 0; i < current_level.enemy_instance_count; i++)
		update_enemy_instance(current_level.enemy_instances + i, player, weapon);
}

void deinit_enemy(const Enemy* const enemy) {
	deinit_sprite(enemy -> animation_data.sprite);
	for (byte i = 0; i < 5; i++) deinit_sound(enemy -> sounds + i);
}
