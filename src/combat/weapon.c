////////// Hitscanning is separate from DDA because DDA inherently steps on whole grids, while weapons do not

typedef struct {
	vec pos;
	const vec dir;
	double dist;
	const double step; // the magnitude of the velocity vector
} Hitscan;

inlinable byte iter_hitscan(Hitscan* const hitscan) {
	hitscan -> pos += hitscan -> dir * vec_fill(hitscan -> step);
	hitscan -> dist += hitscan -> step;
	return !vec_out_of_bounds(hitscan -> pos);
}

//////////

static const double weapon_hitscan_step = 0.3;
static const vec projectile_size = {0.2, 0.2};

void deinit_weapon(const Weapon* const weapon) {
	deinit_sound(&weapon -> sound);
	deinit_sprite(weapon -> animation_data.immut.sprite);
}

#ifndef NOCLIP_MODE

#ifdef DISABLE_ENEMIES
#define shoot_weapon(a, b, c, d)
#else

// the whip doesn't work up close

static void shoot_weapon(const Weapon* const weapon, const vec p_pos, const vec p_dir, const double p_height) {
	(void) p_height;

	Hitscan hitscan = {p_pos, p_dir, 0.0, weapon_hitscan_step};
	const byte short_range_weapon = bit_is_set(weapon -> status, mask_short_range_weapon);

	while (iter_hitscan(&hitscan)) {
		const BoundingBox projectile_box = init_bounding_box(hitscan.pos, projectile_size); 
		byte collided = 0;

		for (byte i = 0; i < current_level.enemy_instance_count; i++) {
			EnemyInstance* const enemy_instance = &current_level.enemy_instances[i];
			if (enemy_instance -> state == Dead || !bit_is_set(enemy_instance -> status, mask_weapon_y_pitch_in_range_of_enemy))
				continue;

			const BoundingBox enemy_box = init_bounding_box(enemy_instance -> billboard_data.pos, vec_fill(actor_box_side_len));
			if (aabb_collision(projectile_box, enemy_box)) {
				set_bit(enemy_instance -> status, mask_recently_attacked_enemy);
				enemy_instance -> hp -= weapon -> power;

				void set_enemy_instance_state(EnemyInstance* const, const EnemyState, const byte, const vec, const double);

				if (enemy_instance -> hp <= 0.0)
					set_enemy_instance_state(enemy_instance, Dead, 0, p_pos, p_height);
				else
					play_sound_from_billboard_data(
					&enemy_instance -> enemy -> sounds[4], // attacked
					&enemy_instance -> billboard_data, p_pos, p_height);

				collided = 1;
			}
		}
		if (collided || short_range_weapon) break;
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
		shoot_weapon(weapon, player -> pos, player -> dir, player -> jump.height);
	}
	else clear_bit(weapon -> status, mask_recently_used_weapon); // recently used = within the last tick

	// -1 -> cycle frame, 0 -> first frame
	animate_weapon(&weapon -> animation_data, player -> pos, bit_is_set(weapon -> status, mask_paces_sideways_weapon),
		bit_is_set(weapon -> status, mask_in_use_weapon), player -> body.v);
}

#else

#define use_weapon_if_needed(a, b, c)

#endif
