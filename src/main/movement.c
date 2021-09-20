static const double
	thing_box_side_len = 0.5, // the player's bounding box has the same size
	min_fall_height_for_sound = 2.0;

typedef struct {
	const vec origin, size; // origin = top left corner
} BoundingBox;

byte aabb_axis_collision(const BoundingBox a, const BoundingBox b, const byte axis) {
	return (a.origin[axis] < b.origin[axis] + b.size[axis]) && (a.origin[axis] + a.size[axis] > b.origin[axis]);
}

inlinable byte aabb_collision(const BoundingBox a, const BoundingBox b) {
	return aabb_axis_collision(a, b, 0) && aabb_axis_collision(a, b, 1);
}

inlinable BoundingBox bounding_box_from_pos(const vec pos, const vec size) {
	return (BoundingBox) {pos - size * vec_fill(0.5), size};
}

// aabb = axis-aligned bounding box
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

	const vec box_dimensions = vec_fill(thing_box_side_len);

	const BoundingBox player_boxes[2] = {
		bounding_box_from_pos(pos_change_with_x, box_dimensions),
		bounding_box_from_pos(pos_change_with_y, box_dimensions)
	};

	for (byte i = 0; i < current_level.thing_count; i++) {
		const DataBillboard* const billboard_data = current_level.thing_container[i].billboard_data;

		const double y_delta = fabs(billboard_data -> height - p_height);
		if (y_delta >= 1.0) continue;

		const BoundingBox thing_box = bounding_box_from_pos(billboard_data -> pos, box_dimensions);

		if (aabb_collision(thing_box, player_boxes[0])) *hit_x = 1;
		if (aabb_collision(thing_box, player_boxes[1])) *hit_y = 1;

		if (*hit_x && *hit_y) break;
	}
}

void update_pos(vec* const ref_pos, vec* const dir,
	KinematicBody* const body, const double rad_theta, const double p_height,
	const byte forward, const byte backward, const byte lstrafe, const byte rstrafe) {

	const double curr_time = SDL_GetTicks() / 1000.0; // in seconds
	byte increasing_fov = 0;

	if (bit_is_set(body -> status, mask_forward_or_backward_movement)) {
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

		body -> max_v_reached = body -> v,
		nth_bit_to_x(&body -> status, 1, forward);
		nth_bit_to_x(&body -> status, 2, backward);
	}

	else {
		const double t = curr_time - body -> time_of_stop;
		body -> v = body -> max_v_reached - body -> a * t;
		if (body -> v < 0.0) body -> v = 0.0;
	}

	if (!increasing_fov && settings.fov > INIT_FOV)
		update_fov(settings.fov - settings.fov_step);

	*dir = (vec) {cos(rad_theta), sin(rad_theta)};

	const vec
		forward_back_movement = *dir * vec_fill(body -> v),
		sideways_movement = *dir * vec_fill(body -> strafe_v);

	vec movement = {0.0, 0.0};

	if (bit_is_set(body -> status, mask_forward_movement)) movement += forward_back_movement;
	else if (bit_is_set(body -> status, mask_backward_movement)) movement -= forward_back_movement;

	if (lstrafe) movement[0] += sideways_movement[1], movement[1] -= sideways_movement[0];
	if (rstrafe) movement[0] -= sideways_movement[1], movement[1] += sideways_movement[0];

	////////// collision detection
	#ifdef NOCLIP_MODE
	(void) p_height;
	*ref_pos += movement;

	#else
	byte thing_hit_x, thing_hit_y;
	vec pos = *ref_pos;
	report_aabb_thing_collisions(pos, movement, &thing_hit_x, &thing_hit_y, p_height);

	if (!point_exists_at(pos[0] + movement[0], pos[1], p_height) && !thing_hit_x)
		pos[0] += movement[0];
	if (!point_exists_at(pos[0], pos[1] + movement[1], p_height) && !thing_hit_y)
		pos[1] += movement[1];

	*ref_pos = pos;
	#endif
	//////////
}

inlinable void init_a_jump(Jump* const jump, const byte falling) {
	jump -> jumping = 1;
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

	if (keys[KEY_JUMP] && !jump -> jumping) {
		init_a_jump(jump, 0);
		play_sound(&jump -> sound_at_jump, 0);
		jump -> made_noise = 1;
	}
	else jump -> made_noise = 0;
	
	//////////
	double ground_height;
	static byte first_call = 1;
	byte landed_on_thing = 0;

	#ifndef PLANAR_MODE
	byte hit_head = 0;

	if (!first_call) { // first_call avoided for the same reason as explained in handle_thing_collisions
		for (byte i = 0; i < current_level.thing_count; i++) {
			const Thing* const thing = &current_level.thing_container[i];
			if (!bit_is_set(thing -> status, mask_can_jump_on_thing)) continue;

			const DataBillboard* const billboard_data = thing -> billboard_data;
			const double thing_height = billboard_data -> height;

			if (!vec_delta_exceeds(billboard_data -> pos, pos, thing_box_side_len)) {
				const double top_thing_height = thing_height + 1.0;
				if (jump -> height >= top_thing_height) {
					landed_on_thing = 1;
					ground_height = top_thing_height;
					break;
				}
				else if (jump -> height + 1.0 > thing_height) {
					hit_head = 1;
					break;
				}
			}
		}
	}
	#endif

	if (!landed_on_thing) {
		const byte point = *map_point(current_level.wall_data, pos[0], pos[1]);
		ground_height = current_level.get_point_height(point, pos);
	}

	first_call = 0;
	//////////

	if (jump -> jumping) {
		const double t = SDL_GetTicks() / 1000.0 - jump -> time_at_jump;

		// y = y0 + v0t + 0.5at^2
		jump -> height = jump -> start_height + ((jump -> v0 * t) + (0.5 * g * (t * t)));

		if (jump -> height > jump -> highest_height)
			jump -> highest_height = jump -> height;

		if (jump -> height < ground_height) { // v = v0 + at
			if (jump -> highest_height > ground_height && (jump -> v0 + g * t) < 0.0) { // reset jump
				jump -> jumping = 0;
				jump -> start_height = ground_height;
				jump -> height = ground_height;

				// for big jumps only
				if (jump -> highest_height - ground_height >= min_fall_height_for_sound) {
					play_sound(&jump -> sound_at_land, 0);
					jump -> made_noise = 1;
				}
				else jump -> made_noise = 0;

				jump -> highest_height = jump -> height; // + 0.001;
			}
		}
	}

	else if (ground_height < jump -> height) { // falling from running off an object
		init_a_jump(jump, 1);
	}

	#endif

	if (jump -> height < 0.0) jump -> height = 0.0;
}
