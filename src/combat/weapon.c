// Tracing is separate from DDA because DDA inherently steps on whole grids, while tracers do not

static const float
	short_range_tracer_step = 0.3, // the magnitude of the velocity vector
	long_range_tracer_step = 0.1,
	long_range_projectile_tracer_step = 0.12,
	hitscan_projectile_size = 0.2,
	inter_tick_projectile_size = 1.0;

Sprite projectile_sprite;

void init_projectile_resources(void) {
	projectile_sprite = init_sprite("assets/objects/fireball.bmp", 0);
}

void deinit_projectile_resources(void) {
	deinit_sprite(projectile_sprite);
}

void deinit_weapon(const Weapon* const weapon) {
	deinit_sound(&weapon -> sound);
	deinit_sprite(weapon -> animation_data.immut.sprite);
}

inlinable Tracer init_tracer_from_player(const Player* const player, const float step, const byte is_hitscan) {
	const vec p_pos = player -> pos, p_dir = player -> dir; // these are 2D

	return (Tracer) { // Shoots from center of player
		{p_pos[0], p_pos[1], player -> jump.height + actor_eye_height},
		{p_dir[0], p_dir[1], atan((player -> y_pitch + player -> pace.screen_offset) / settings.proj_dist)},
		0.0, step, is_hitscan
	};
}

// Returns if tracing should continue
inlinable byte iter_tracer(Tracer* const tracer) {
	const float step = tracer -> step;

	tracer -> dist += step;
	const vec3D new_pos = tracer -> pos + tracer -> dir * vec_fill_3D(step);
	tracer -> pos = new_pos;

	float height = new_pos[2];

	const byte above_ground = (tracer -> is_hitscan) ? (height >= 0.0f) : (height >= 0.5f);
	return above_ground && !point_exists_at((double) new_pos[0], (double) new_pos[1], (double) height);
}

inlinable void update_inter_tick_projectiles(void) {
	byte new_projectile_count = current_level.projectile_count;
	for (byte i = 0; i < current_level.projectile_count; i++) {
		Projectile* const projectile_ref = current_level.projectiles + i;
		const int channel = projectile_ref -> sound_channel;
		if (!iter_tracer(&projectile_ref -> tracer) || !channel_still_playing(channel)) {

			current_level.thing_count--;
			new_projectile_count--;
			/* Below, all projectiles on the right side of the current element are shifted left by 1,
			essentially deleting the projectile at position `i` */
			memmove(projectile_ref, projectile_ref + 1,
				(current_level.projectile_count - i - 1) * sizeof(Projectile));

			// Do a boom noise here + check for thing collisions

			stop_sound_channel(channel);
		}
		else update_channel_from_dist_3D_and_beta(
			channel, quietest_projectile_sound_dist,
			(double) projectile_ref -> tracer.dist,
			projectile_ref -> billboard_data.beta);
	}
	current_level.projectile_count = new_projectile_count;
}

#ifdef NOCLIP_MODE

#define use_weapon_if_needed(a, b, c)

#else

static void use_inter_tick_projectile_weapon(const Weapon* const weapon, const Player* const player, const int channel) {
	(void) weapon;

	if (current_level.projectile_count == current_level.alloc_projectile_count) {
		current_level.projectiles = wrealloc(current_level.projectiles,
			++current_level.alloc_projectile_count * sizeof(Projectile));

		current_level.thing_container = wrealloc(current_level.thing_container,
			++current_level.alloc_thing_count * sizeof(Thing));
	}

	const Tracer tracer = init_tracer_from_player(player, long_range_projectile_tracer_step, 0);

	const Projectile projectile = {
		.billboard_data = {
			.pos = {(double) tracer.pos[0], (double) tracer.pos[1]},
			.height = player -> jump.height
		},
		.tracer = tracer,
		.sound_channel = channel
	};

	memcpy(current_level.projectiles + current_level.projectile_count, &projectile, sizeof(Projectile));

	current_level.thing_count++;
	current_level.projectile_count++;

	/*
	- start the projectile a bit out from the player
	- for p in projectiles:
		if it hit an enemy or wall, dec thing count and projectile count
		if it hit an enemy, reduce the enemy health some, and make a noise or some cool effect
	*/
}

static void use_hitscan_weapon(const Weapon* const weapon, const Player* const player) {
	const vec p_pos = player -> pos;
	const double p_height = player -> jump.height;

	const byte short_range_weapon = bit_is_set(weapon -> flags, mask_short_range_weapon);

	Tracer tracer = init_tracer_from_player(player,
		short_range_weapon ? short_range_tracer_step : long_range_tracer_step, 1);

	while (iter_tracer(&tracer)) {
		const BoundingBox_3D projectile_box = init_bounding_box_3D(tracer.pos, hitscan_projectile_size);

		byte collided = 0;

		for (byte i = 0; i < current_level.enemy_instance_count; i++) {
			EnemyInstance* const enemy_instance = current_level.enemy_instances + i;
			if (enemy_instance -> state == Dead) continue;

			const BoundingBox_3D enemy_box = init_actor_bounding_box(
				enemy_instance -> billboard_data.pos,
				enemy_instance -> billboard_data.height);

			if (aabb_collision_3D(projectile_box, enemy_box)) {
				set_bit(enemy_instance -> flags, mask_recently_attacked_enemy);

				void set_enemy_instance_state(EnemyInstance* const,
					const EnemyState, const byte, const vec, const double);

				// `f(x) = 1.0 - (log2(x) / 8)`, range zero and below to infinity (smoothly decreasing slope)
				double percent_damage = 1.0 - (log2((double) tracer.dist) / 8.0);
				if (percent_damage > 1.0) percent_damage = 1.0;
				else if (percent_damage < 0.0) percent_damage = 0.0;

				if ((enemy_instance -> hp -= weapon -> power) <= 0.0)
					set_enemy_instance_state(enemy_instance, Dead, 0, p_pos, p_height);
				else {
					const int channel = play_short_sound(enemy_instance -> enemy -> sounds + 4); // attacked
					update_channel_from_thing_billboard_data(channel, &enemy_instance -> billboard_data, p_pos, p_height);
				}

				collided = 1;
			}
		}
		if (collided || short_range_weapon) break;
	}
}

void use_weapon_if_needed(Weapon* const weapon, const Player* const player, const InputStatus input_status) {
	int* const frame_ind = &weapon -> animation_data.mut.frame_ind;

	if (player -> is_dead) *frame_ind = 0;

	const byte
		first_in_use = bit_is_set(weapon -> flags, mask_in_use_weapon),
		spawns_projectile = bit_is_set(weapon -> flags, mask_spawns_projectile_weapon);

	if (first_in_use && *frame_ind == 0)
		clear_bit(weapon -> flags, mask_in_use_weapon);
	else if (input_status == BeginAnimatingWeapon && !first_in_use && !player -> is_dead) {
		set_bit(weapon -> flags, mask_in_use_weapon | mask_recently_used_weapon);
		const int channel = play_short_sound(&weapon -> sound);
		if (spawns_projectile) use_inter_tick_projectile_weapon(weapon, player, channel);
		else use_hitscan_weapon(weapon, player);
	}
	else clear_bit(weapon -> flags, mask_recently_used_weapon); // recently used = within the last tick
}

#endif
