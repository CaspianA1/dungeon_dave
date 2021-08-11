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

		if (moved_any_direction && !(body -> status & mask_forward_or_backward))
			body -> time_of_move = curr_secs;
		else if (!moved_any_direction && (body -> status & mask_forward_or_backward))
			body -> time_of_stop = curr_secs;

		nth_bit_to_x(&body -> status, 0, moved_forward_or_backward);

		double* const theta = &player -> angle;
		vec* const pos = &player -> pos;
		const vec prev_pos = *pos;
		ivec* const mouse_pos = &player -> mouse_pos;

		update_mouse_and_theta(theta, mouse_pos);
		update_pos(pos, &player -> dir, body, to_radians(*theta),
			player -> jump.height, forward, backward, lstrafe, rstrafe);

		update_jump(&player -> jump, player -> pos);
		update_y_pitch(&player -> y_pitch, mouse_pos -> y);
		update_tilt(&player -> tilt, strafe, lstrafe);
		update_pace(&player -> pace, *pos, prev_pos, player -> body.v, player -> body.limit_v);
	}

	return input_status;
}
