static const struct {
	const byte backward, forward, forward_or_backward;
} kinematic_body_masks = {0b00000100, 0b00000010, 0b00000001};

#define nth_to_x(num, n, x) num ^= (-x ^ num) & (1 << n)

inlinable void update_mouse_and_theta(double* const theta, ivec* const mouse_pos) {
	const int prev_mouse_x = mouse_pos -> x;
	SDL_GetMouseState(&mouse_pos -> x, &mouse_pos -> y);
	if (prev_mouse_x == mouse_pos -> x) return;

	*theta += (double) (mouse_pos -> x - prev_mouse_x) / settings.screen_width * 360.0;

	if (mouse_pos -> x == settings.screen_width - 1)
		SDL_WarpMouseInWindow(screen.window, 1, mouse_pos -> y);
	else if (mouse_pos -> x == 0)
		SDL_WarpMouseInWindow(screen.window, settings.screen_width - 1, mouse_pos -> y);
}

void hit_detection(vec* const pos_ref, const vec prev_pos, const vec movement, const double p_height) {
	vec pos = *pos_ref + movement;

	#ifdef NOCLIP_MODE

	(void) prev_pos;
	(void) p_height;

	/* Out-of-bounds hit detection is only needed for noclip mode,
	as it will be impossible to go out of bounds otherwise in normal mode. */
	if (pos[1] < 1 || pos[1] > current_level.map_size.y - 1) pos[1] = prev_pos[1];
	if (pos[0] < 1 || pos[0] > current_level.map_size.x - 1) pos[0] = prev_pos[0];

	#else

	if (point_exists_at(pos[0], pos[1] + settings.stop_dist, p_height) ||
		point_exists_at(pos[0], pos[1] - settings.stop_dist, p_height))
		pos[1] = prev_pos[1]; // set pos to block-aligned value?

	if (point_exists_at(pos[0] + settings.stop_dist, pos[1], p_height) ||
		point_exists_at(pos[0] - settings.stop_dist, pos[1], p_height))
		pos[0] = prev_pos[0];

	#endif

	*pos_ref = pos;
}

void update_pos(vec* const pos, const vec prev_pos, vec* const dir,
	KinematicBody* const body, const double rad_theta, const double p_height,
	const byte forward, const byte backward, const byte lstrafe, const byte rstrafe) {

	const double curr_time = SDL_GetTicks() / 1000.0; // in seconds
	byte increasing_fov = 0;

	if (body -> status & kinematic_body_masks.forward_or_backward) {
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
		nth_to_x(body -> status, 1, forward);
		nth_to_x(body -> status, 2, backward);
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

	if (body -> status & kinematic_body_masks.forward) movement += forward_back_movement;
	if (body -> status & kinematic_body_masks.backward) movement -= forward_back_movement;

	if (lstrafe) movement[0] += sideways_movement[1], movement[1] -= sideways_movement[0];
	if (rstrafe) movement[0] -= sideways_movement[1], movement[1] += sideways_movement[0];

	hit_detection(pos, prev_pos, movement, p_height);
}

void update_y_pitch(int* const y_pitch, const int mouse_y) {
	*y_pitch = -mouse_y + settings.half_screen_height;
}

inlinable void update_tilt(Domain* const tilt, const byte strafe, const byte lstrafe) {
	if (strafe) {
		tilt -> val += lstrafe ? tilt -> step : -tilt -> step;

		if (tilt -> val > tilt -> max)
			tilt -> val = tilt -> max;
		else if (tilt -> val < -tilt -> max)
			tilt -> val = -tilt -> max;
	}

	else if (tilt -> val + tilt -> step < 0.0) tilt -> val += tilt -> step;
	else if (tilt -> val - tilt -> step > 0.0) tilt -> val -= tilt -> step;
}

#ifdef NOCLIP_MODE

#define update_pace(a, b, c, d, e)

#else

inlinable void update_pace(Pace* const pace, const vec pos, const vec prev_pos, const double v, const double lim_v) {
	if (pos[0] != prev_pos[0] || pos[1] != prev_pos[1]) {
		Domain* const domain = &pace -> domain;
		const double domain_incr = domain -> step * log(lim_v) / log(v);
		if ((domain -> val += domain_incr) > two_pi) domain -> val = 0.0;
		// the magical number 0.98 causes skybox warping problems to disappear
		pace -> screen_offset = (cos(domain -> val) - 0.98) * settings.screen_height / pace -> offset_scaler;
	}
}

#endif

inlinable void init_a_jump(Jump* const jump, const byte falling) {
	jump -> jumping = 1;
	jump -> time_at_jump = SDL_GetTicks() / 1000.0;
	jump -> start_height = jump -> height;
	jump -> highest_height = jump -> height;
	jump -> v0 = falling ? 0 : jump -> up_v0;
}

void update_jump(Jump* const jump, const vec pos) {
	const double min_fall_height_for_sound = 2.0;

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
		play_sound(jump -> sound_at_jump, 0);
		jump -> made_noise = 1;
	}
	else jump -> made_noise = 0;

	const byte point = map_point(current_level.wall_data, pos[0], pos[1]);
	const byte wall_point_height = current_level.get_point_height(point, pos);

	if (jump -> jumping) {
		const double t = SDL_GetTicks() / 1000.0 - jump -> time_at_jump;

		// y = y0 + v0t + 0.5at^2
		jump -> height = jump -> start_height + ((jump -> v0 * t) + (0.5 * g * (t * t)));

		if (jump -> height > jump -> highest_height)
			jump -> highest_height = jump -> height;

		if (jump -> height < wall_point_height) {
			if (jump -> highest_height > wall_point_height && (jump -> v0 + g * t) < 0.0) {
				jump -> jumping = 0;
				jump -> start_height = wall_point_height;
				jump -> height = wall_point_height;

				// for big jumps only
				if (jump -> highest_height - wall_point_height >= min_fall_height_for_sound) {
					play_sound(jump -> sound_at_land, 0);
					jump -> made_noise = 1;
				}
				else jump -> made_noise = 0;

				jump -> highest_height = jump -> height + 0.001;
			}
		}
	}

	else if (wall_point_height < jump -> height) // falling
		init_a_jump(jump, 1);

	#endif

	if (jump -> height < 0.0) jump -> height = 0.0;
}

InputStatus handle_input(Player* const player, const byte restrict_movement) {
	InputStatus input_status = ProceedAsNormal;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_CLOSE)
					input_status = Exit;
				break;

			case SDL_MOUSEBUTTONDOWN:
				if (event.button.button == KEY_USE_WEAPON)
					input_status = BeginAnimatingWeapon;
		}
	}

	if (!restrict_movement) {
		//////////
		const byte forward = keys[KEY_FORWARD], backward = keys[KEY_BACKWARD],
			lstrafe = keys[KEY_LSTRAFE], rstrafe = keys[KEY_RSTRAFE];

		const byte
			strafe = lstrafe ^ rstrafe, // only strafe if one is true
			moved_forward_or_backward = forward ^ backward;

		const byte moved_any_direction = strafe || moved_forward_or_backward;
		//////////

		KinematicBody* const body = &player -> body;
		const double curr_secs = SDL_GetTicks() / 1000.0;

		if (moved_any_direction && !(body -> status & kinematic_body_masks.forward_or_backward))
			body -> time_of_move = curr_secs;
		else if (!moved_any_direction && (body -> status & kinematic_body_masks.forward_or_backward))
			body -> time_of_stop = curr_secs;

		nth_to_x(body -> status, 0, moved_forward_or_backward);

		double* const theta = &player -> angle;
		vec* const pos = &player -> pos;
		const vec prev_pos = *pos;
		ivec* const mouse_pos = &player -> mouse_pos;

		update_mouse_and_theta(theta, mouse_pos);
		update_pos(pos, prev_pos, &player -> dir, body, to_radians(*theta),
			player -> jump.height, forward, backward, lstrafe, rstrafe);

		update_jump(&player -> jump, player -> pos);
		update_y_pitch(&player -> y_pitch, mouse_pos -> y);
		update_tilt(&player -> tilt, strafe, lstrafe);
		update_pace(&player -> pace, *pos, prev_pos, player -> body.v, player -> body.limit_v);
	}

	return input_status;
}
