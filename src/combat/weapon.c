static const double
	weapon_dda_step = 0.3,
	weapon_max_hit_dist = 0.5;

static const vec projectile_size = {0.3, 0.3};

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
	DataDDA bullet = init_dda(p_pos, p_dir, weapon_dda_step);
	const byte is_short_range = bit_is_set(weapon -> status, mask_short_range_weapon);

	while (iter_dda(&bullet)) {
		const vec projectile_pos = vec_line_pos(p_pos, p_dir, bullet.dist);
		const BoundingBox projectile_box = init_bounding_box(projectile_pos, projectile_size);

		const byte point = *map_point(current_level.wall_data, projectile_pos[0], projectile_pos[1]);
		if (current_level.get_point_height(point, (vec) {projectile_pos[0], projectile_pos[1]}) > p_height) break;

		byte collided = 0;
		for (byte i = 0; i < current_level.enemy_instance_count; i++) {
			EnemyInstance* const enemy_instance = &current_level.enemy_instances[i];
			if (enemy_instance -> state == Dead) continue;
			const BoundingBox enemy_box = init_bounding_box(enemy_instance -> billboard_data.pos, vec_fill(actor_box_side_len));

			if (aabb_collision(projectile_box, enemy_box) &&
				bit_is_set(enemy_instance -> status, mask_weapon_y_pitch_in_range_of_enemy)) {

				set_bit(enemy_instance -> status, mask_recently_attacked_enemy);
				enemy_instance -> hp -= weapon -> power;

				void set_enemy_instance_state(EnemyInstance* const, const EnemyState, const byte);
				if (enemy_instance -> hp <= 0.0) set_enemy_instance_state(enemy_instance, Dead, 0);
				else play_sound(&enemy_instance -> enemy -> sounds[4], 0); // attacked

				collided = 1;
			}
		}
		if (collided || is_short_range) break;
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
		play_sound(&weapon -> sound, 0);
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
