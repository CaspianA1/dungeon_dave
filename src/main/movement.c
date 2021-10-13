inlinable void report_aabb_thing_collisions(const vec pos, const vec movement,
	byte* const hit_x, byte* const hit_y, const double p_height) { 

	*hit_x = 0;
	*hit_y = 0;

	#ifdef PLANAR_MODE
	return;
	#endif

	static byte first_call = 1; // first call ignored b/c thing data has not been initialized yet
	if (first_call) {
		first_call = 0;
		return;
	}

	vec pos_change_with_x = pos, pos_change_with_y = pos;
	pos_change_with_x[0] += movement[0];
	pos_change_with_y[1] += movement[1];

	const BoundingBox player_boxes[2] = {
		init_bounding_box(pos_change_with_x, actor_box_side_len),
		init_bounding_box(pos_change_with_y, actor_box_side_len)
	};

	for (byte i = 0; i < current_level.thing_count; i++) {
		const Thing* const thing = current_level.thing_container + i;
		if (bit_is_set(thing -> flags, mask_can_move_through_thing)) continue;

		const DataBillboard* const billboard_data = thing -> billboard_data;
		const double y_delta = fabs(billboard_data -> height - p_height);
		if (y_delta >= actor_height) continue;

		const BoundingBox thing_box = init_bounding_box(billboard_data -> pos, actor_box_side_len);

		if (aabb_collision(thing_box, player_boxes[0])) *hit_x = 1;
		if (aabb_collision(thing_box, player_boxes[1])) *hit_y = 1;

		if (*hit_x && *hit_y) break;
	}
}

void update_pos(vec* const pos, vec* const dir,
	KinematicBody* const body, const double theta, const double p_height,
	const byte forward, const byte backward, const byte lstrafe, const byte rstrafe) {

	const double curr_time = SDL_GetTicks() / 1000.0; // in seconds
	byte increasing_fov = 0;

	if (bit_is_set(body -> flags, mask_forward_or_backward_movement)) {
		const double t = curr_time - body -> time_of_move;
		body -> v = body -> a * t;

		if (keys[KEY_SPEEDUP_1] || keys[KEY_SPEEDUP_2]) {
			increasing_fov = 1;
			body -> v *= body -> v_incr_multiplier;

			if (settings.fov < settings.max_fov)
				update_fov(settings.fov + settings.fov_step);
		}

		if (body -> v > body -> limit_v)
			body -> v = body -> limit_v;

		body -> max_v_reached = body -> v;

		bit_to_x(body -> flags, mask_forward_movement, forward);
		bit_to_x(body -> flags, mask_backward_movement, backward);
	}

	else {
		const double t = curr_time - body -> time_of_stop;
		body -> v = body -> max_v_reached - body -> a * t;
		if (body -> v < 0.0) body -> v = 0.0;
	}

	if (!increasing_fov && settings.fov > settings.init_fov)
		update_fov(settings.fov - settings.fov_step);

	*dir = (vec) {cos(theta), sin(theta)};

	const vec
		forward_back_movement = *dir * vec_fill(body -> v),
		sideways_movement = *dir * vec_fill(body -> strafe_v);

	vec movement = {0.0, 0.0};

	if (bit_is_set(body -> flags, mask_forward_movement)) movement += forward_back_movement;
	else if (bit_is_set(body -> flags, mask_backward_movement)) movement -= forward_back_movement;

	if (lstrafe) movement[0] += sideways_movement[1], movement[1] -= sideways_movement[0];
	if (rstrafe) movement[0] -= sideways_movement[1], movement[1] += sideways_movement[0];

	////////// Collision detection
	#ifdef NOCLIP_MODE
	(void) p_height;
	*pos += movement;
	#else
	byte thing_hit_x, thing_hit_y;
	vec new_pos = *pos;
	const vec orig_pos = new_pos;
	report_aabb_thing_collisions(new_pos, movement, &thing_hit_x, &thing_hit_y, p_height);

	if (!point_exists_at(new_pos[0] + movement[0], new_pos[1], p_height) && !thing_hit_x)
		new_pos[0] += movement[0];
	if (!point_exists_at(new_pos[0], new_pos[1] + movement[1], p_height) && !thing_hit_y)
		new_pos[1] += movement[1];

	if (new_pos[0] == orig_pos[0] && new_pos[1] == orig_pos[1])
		body -> v = 0.0;

	*pos = new_pos;
	#endif
	//////////
}

inlinable void init_a_jump(Jump* const jump, const byte falling) {
	set_bit(jump -> flags, mask_currently_jumping);
	jump -> time_at_jump = SDL_GetTicks() / 1000.0;
	jump -> start_height = jump -> height;
	jump -> highest_height = jump -> height;
	jump -> v0 = falling ? 0.0 : jump -> up_v0;
}

void update_jump(Jump* const jump, const vec pos) {
	#ifdef NOCLIP_MODE

	(void) pos;

	double* const
		last_tick_time = &jump -> time_at_jump,
		curr_tick_time = SDL_GetTicks() / 1000.0;

	const double pos_change = jump -> up_v0 * (curr_tick_time - *last_tick_time);

	if (keys[KEY_FLY_UP]) jump -> height += pos_change;
	if (keys[KEY_FLY_DOWN]) jump -> height -= pos_change;

	*last_tick_time = curr_tick_time;

	#else

	const byte starting_jump = keys[KEY_JUMP] && !bit_is_set(jump -> flags, mask_currently_jumping);
	bit_to_x(jump -> flags, mask_made_noise_jump, starting_jump);
	if (starting_jump) {
		init_a_jump(jump, 0);
		play_sound(&jump -> sound_at_jump);
	}

	//////////
	double ground_height;
	static byte first_call = 1;
	byte landed_on_thing = 0;

	#ifndef PLANAR_MODE
	byte hit_head = 0;

	if (!first_call) { // first_call avoided for the same reason as explained in handle_thing_collisions
		for (byte i = 0; i < current_level.thing_count; i++) {
			const Thing* const thing = current_level.thing_container + i;
			if (bit_is_set(thing -> flags, mask_can_move_through_thing)) continue;

			const DataBillboard* const billboard_data = thing -> billboard_data;
			const double thing_height = billboard_data -> height;

			/* Circular approximation of closeness b/c AABB collisions
			are resolved otherwise and won't stay across ticks */
			if (!vec_delta_exceeds(billboard_data -> pos, pos, actor_box_side_len)) {
				const double top_thing_height = thing_height + actor_height;

				if (jump -> height >= top_thing_height) {
					landed_on_thing = 1;
					ground_height = top_thing_height;
					break;
				}
				else if (jump -> height + actor_height > thing_height) {
					hit_head = 1;
					break;
				}
			}
		}
	}
	#endif

	if (!landed_on_thing) ground_height = *map_point(current_level.heightmap, pos[0], pos[1]);
	first_call = 0;
	//////////
	if (bit_is_set(jump -> flags, mask_currently_jumping)) {
		const double t = SDL_GetTicks() / 1000.0 - jump -> time_at_jump;

		// y = y0 + v0t + 0.5at^2
		jump -> height = jump -> start_height + ((jump -> v0 * t) + (0.5 * g * (t * t)));

		if (jump -> height > jump -> highest_height)
			jump -> highest_height = jump -> height;

		else if (jump -> height < ground_height) { // v = v0 + at
			if (jump -> highest_height > ground_height && (jump -> v0 + g * t) < 0.0) { // reset jump
				clear_bit(jump -> flags, mask_currently_jumping);
				jump -> start_height = ground_height;
				jump -> height = ground_height;

				const byte made_noise = jump -> highest_height - ground_height >= min_fall_height_for_sound;
				bit_to_x(jump -> flags, mask_made_noise_jump, made_noise);
				if (made_noise) play_sound(&jump -> sound_at_land);

				jump -> highest_height = jump -> height;
			}
		}
	}

	// Falling from running off of an object
	else if (ground_height < jump -> height) init_a_jump(jump, 1);
	#endif
}
