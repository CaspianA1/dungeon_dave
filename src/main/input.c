inlinable void update_theta_and_y_pitch(double* const ref_theta, int* const ref_y_pitch) {
	ivec mouse_delta;
	SDL_GetRelativeMouseState(&mouse_delta.x, &mouse_delta.y);

	double theta = *ref_theta + ((double) mouse_delta.x / settings.screen_width * two_pi);
	if (theta > two_pi) theta = 0.0;
	else if (theta < 0.0) theta = two_pi;
	*ref_theta = theta;

	int y_pitch = *ref_y_pitch - mouse_delta.y;
	if (y_pitch > settings.half_screen_height) y_pitch = settings.half_screen_height;
	else if (y_pitch < -settings.half_screen_height) y_pitch = -settings.half_screen_height;

	*ref_y_pitch = y_pitch;
}

inlinable void update_tilt(Domain* const tilt, const byte strafe, const byte lstrafe) {
	double tilt_val = tilt -> val;
	const double tilt_step = tilt -> step;
	const char tilt_dir = (lstrafe << 1) - strafe; // 1 -> left, 0 -> none, -1 -> right

	if (strafe) {
		const double old_tilt = tilt_val, tilt_max = tilt -> max;
		tilt_val += tilt_step * tilt_dir;
		if (tilt_val >= tilt_max || tilt_val <= -tilt_max) tilt_val = old_tilt;
	}

	else if (!doubles_eq(tilt_val, 0.0)) {
		const char readjust_tilt_dir = (tilt_val < 0.0) ? 1 : -1;
		tilt_val += tilt_step * readjust_tilt_dir;
	}

	tilt -> val = tilt_val;
}

#ifdef NOCLIP_MODE

#define update_pace(a, b, c, d, e) (void) c

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

InputStatus handle_input(Player* const player, const byte restrict_movement) {
	InputStatus input_status = ProceedAsNormal;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_CLOSE)
					input_status = Exit;
				break;

			case SDL_MOUSEBUTTONDOWN:
				if (event.button.button == KEY_USE_WEAPON) input_status = BeginAnimatingWeapon;
				break;

			default: if (keys[KEY_TOGGLE_OPTIONS_MENU]) input_status = OptionsMenu;
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

		const byte forward_or_backward = bit_is_set(body -> status, mask_forward_or_backward_movement);

		if (moved_any_direction && !forward_or_backward)
			body -> time_of_move = curr_secs;
		else if (!moved_any_direction && forward_or_backward)
			body -> time_of_stop = curr_secs;

		bit_to_x(body -> status, mask_forward_or_backward_movement, moved_forward_or_backward);

		double* const theta = &player -> angle;
		vec* const pos = &player -> pos;

		const vec prev_pos = *pos;

		update_theta_and_y_pitch(theta, &player -> y_pitch);
		update_pos(pos, &player -> dir, body, *theta, player -> jump.height, forward, backward, lstrafe, rstrafe);

		update_jump(&player -> jump, player -> pos);
		update_tilt(&player -> tilt, strafe, lstrafe);
		update_pace(&player -> pace, *pos, prev_pos, body -> v, body -> limit_v);
	}

	return input_status;
}
