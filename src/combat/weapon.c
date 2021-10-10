////////// Hitscanning is separate from DDA because DDA inherently steps on whole grids, while weapons do not

static const double
	short_range_hitscan_step = 0.3,
	long_range_hitscan_step = 0.1, // the magnitude of the velocity vector
	projectile_size = 0.2;

//////////

void deinit_weapon(const Weapon* const weapon) {
	deinit_sound(&weapon -> sound);
	deinit_sprite(weapon -> animation_data.immut.sprite);
}

#ifndef NOCLIP_MODE

typedef struct {
	vec3D pos; // x, y, z
	const vec3D dir;
	double dist;
	const byte short_range_scan;
} Hitscan;

// make _3D to 3D

// returns if hitscanning should continue
inlinable byte iter_hitscan(Hitscan* const hitscan) {
	const double step = hitscan -> short_range_scan ? short_range_hitscan_step : long_range_hitscan_step;

	hitscan -> dist += step;
	const vec3D new_pos = hitscan -> pos + hitscan -> dir * vec_fill_3D(step);
	hitscan -> pos = new_pos;

	const float height = new_pos[2];
	return (height >= 0.0f) && (!point_exists_at((double) new_pos[0], (double) new_pos[1], (double) height));
}

#ifdef DISABLE_ENEMIES

#define use_hitscan_weapon(a, b)

#else

static void use_hitscan_weapon(const Weapon* const weapon, const Player* const player) {
	const vec p_pos = player -> pos, p_dir = player -> dir; // these are 2D

	const double
		p_height = player -> jump.height,
		p_pitch_angle = atan((player -> y_pitch + player -> pace.screen_offset) / settings.proj_dist);

	Hitscan hitscan = {
		{p_pos[0], p_pos[1], p_height + 0.5}, {p_dir[0], p_dir[1], p_pitch_angle}, 0.0,
		bit_is_set(weapon -> status, mask_short_range_weapon)
	};

	while (iter_hitscan(&hitscan)) {
		const BoundingBox_3D projectile_box = init_bounding_box_3D(hitscan.pos, projectile_size);

		byte collided = 0;

		for (byte i = 0; i < current_level.enemy_instance_count; i++) {
			EnemyInstance* const enemy_instance = current_level.enemy_instances + i;
			if (enemy_instance -> state == Dead) continue;

			const BoundingBox_3D enemy_box = init_actor_bounding_box(
				enemy_instance -> billboard_data.pos,
				enemy_instance -> billboard_data.height);

			if (aabb_collision_3D(projectile_box, enemy_box)) {
				set_bit(enemy_instance -> status, mask_recently_attacked_enemy);

				void set_enemy_instance_state(EnemyInstance* const, const EnemyState,
					const byte, const vec, const double);

				if ((enemy_instance -> hp -= weapon -> power) <= 0.0)
					set_enemy_instance_state(enemy_instance, Dead, 0, p_pos, p_height);
				else
					play_sound_from_billboard_data(
					enemy_instance -> enemy -> sounds + 4, // attacked
					&enemy_instance -> billboard_data, p_pos, p_height);

				collided = 1;
			}
		}
		if (collided || hitscan.short_range_scan) break;
	}
}

#endif

void use_weapon_if_needed(Weapon* const weapon, const Player* const player, const InputStatus input_status) {
	int* const frame_ind = &weapon -> animation_data.mut.frame_ind;

	if (player -> is_dead) *frame_ind = 0;

	const byte first_in_use = bit_is_set(weapon -> status, mask_in_use_weapon);

	if (first_in_use && *frame_ind == 0)
		clear_bit(weapon -> status, mask_in_use_weapon);
	else if (input_status == BeginAnimatingWeapon && !first_in_use && !player -> is_dead) {
		set_bit(weapon -> status, mask_in_use_weapon | mask_recently_used_weapon);
		play_sound(&weapon -> sound);
		use_hitscan_weapon(weapon, player);
	}
	else clear_bit(weapon -> status, mask_recently_used_weapon); // recently used = within the last tick

	// -1 -> cycle frame, 0 -> first frame
	animate_weapon(&weapon -> animation_data, player -> pos,
		bit_is_set(weapon -> status, mask_paces_sideways_weapon),
		bit_is_set(weapon -> status, mask_in_use_weapon), player -> body.v);
}

#else

#define use_weapon_if_needed(a, b, c)

#endif
