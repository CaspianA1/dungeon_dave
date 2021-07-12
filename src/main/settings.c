inlinable void update_proj_dist(void) {
	settings.proj_dist = settings.half_screen_width / tan(to_radians(settings.fov / 2.0));

	/*
	fov = 2 * atan(x / 2f), where x = some number (usually diagonal of film), and f = focal length	
	fov / 2 = atan(x / 2f)
	tan(fov / 2) = x / 2f
	2f * tan(fov / 2) = x
	2f = x / tan(fov / 2)
	f = 0.5x / tan(fov / 2)
	*/
}

// see how other raycasters use the fov to determine things
inlinable void update_fov(const double new_fov) {
	if (new_fov <= 0.0 || new_fov >= 180.0) return;
	settings.fov = new_fov;
	update_proj_dist();
}

inlinable void update_max_fps(const int new_max_fps) {
	if (new_max_fps <= 0) return;
	settings.max_fps = new_max_fps;
	settings.max_delay = 1000.0 / new_max_fps;
}

void init_SDL_buffers(const int new_width, const int new_height, const byte should_free) {
	SDL_Texture** const buffers[2] = {
		&screen.pixel_buffer, &screen.shape_buffer
	};

	const SDL_TextureAccess buffer_access_types[2] = {
		SDL_TEXTUREACCESS_STREAMING, SDL_TEXTUREACCESS_TARGET
	};

	for (byte i = 0; i < 2; i++) {
		SDL_Texture** const buffer = buffers[i];

		if (should_free) SDL_DestroyTexture(*buffer);

		*buffer = SDL_CreateTexture(screen.renderer, PIXEL_FORMAT,
			buffer_access_types[i], new_width, new_height);

		SDL_SetTextureBlendMode(*buffer, SDL_BLENDMODE_BLEND);
	}
}

void load_default_settings(void) {
	settings.screen_width = INIT_W;
	settings.screen_height = INIT_H;
	settings.half_screen_width = settings.screen_width / 2;
	settings.half_screen_height = settings.screen_height / 2;
	update_max_fps(INIT_MAX_FPS);
	settings.ray_column_width = INIT_RAY_COLUMN_W;

	update_fov(INIT_FOV);
	settings.fov_step = INIT_FOV_STEP;
	settings.max_fov = INIT_MAX_FOV;
	settings.minimap_scale = INIT_MINIMAP_SCALE;
	settings.stop_dist_from_wall = INIT_STOP_DIST_FROM_WALL;

	srand(time(NULL));
	keys = SDL_GetKeyboardState(NULL);
}

byte update_screen_dimensions(int* const y_pitch, const int mouse_y) {
	int new_width, new_height;
	SDL_GetWindowSize(screen.window, &new_width, &new_height);

	const byte
		width_not_eq = new_width != settings.screen_width,
		height_not_eq = new_height != settings.screen_height;

	if (width_not_eq || height_not_eq) {
		init_SDL_buffers(new_width, new_height, 1);
		if (width_not_eq) {
			settings.screen_width = new_width;
			settings.half_screen_width = new_width / 2;
			update_proj_dist();

			val_buffer = wrealloc(val_buffer, new_width * sizeof(BufferVal));
		}
		if (height_not_eq) {
			settings.screen_height = new_height;
			settings.half_screen_height = new_height / 2;
			void update_y_pitch(int* const, const int);
			update_y_pitch(y_pitch, mouse_y);
		}
		return 1;
	}
	return 0;
}

Player load_player(const double jump_up_v0,
	const double tilt_step, const double tilt_max, const double pace_step,
	const double pace_offset_scaler, const double body_a, const double body_limit_v,
	const double body_strafe_v, const double body_v_incr_multiplier) {

	const double init_height = current_level.init_height;

	return (Player) {
		.pos = current_level.init_pos,

		.dir = {1.0, 0.0},

		.mouse_pos = {0, 0},

		.angle = 0.0, .y_pitch = 0,

		.jump = {.jumping = 0, .up_v0 = jump_up_v0, .v0 = 0.0,
			.height = init_height, .start_height = init_height,
			.highest_height = init_height, .time_at_jump = 0.0,
			.sound_at_jump = init_sound("../assets/audio/sound_effects/jump_up.wav", 1),
			.sound_at_land = init_sound("../assets/audio/sound_effects/jump_land.wav", 1)},

		.tilt = {.val = 0.0, .step = tilt_step, .max = tilt_max},

		.pace = {.domain = {.val = 0.0, .step = pace_step},
			.offset_scaler = pace_offset_scaler, .screen_offset = 0},

		.body = {.moving_forward_or_backward = 0, .was_forward = 0, .was_backward = 0,
			.v = 0.0, .max_v_reached = 0.0, .a = body_a, .limit_v = body_limit_v,
			.strafe_v = body_strafe_v, .v_incr_multiplier = body_v_incr_multiplier}
	};
}

void load_all_defaults(void (*load_first_level) (void),
	Player* const player, Weapon* const weapon) {

	load_default_settings();

	void init_screen(void);
	init_screen();

	init_audio_subsystem();
	load_first_level();

	const Player first_player = load_player(4.8, 0.3, 8.0, 0.12, 15.0, 0.08, 0.09, 0.05, 1.9);

	const Weapon first_weapon = init_weapon("../assets/audio/sound_effects/shotgun.wav",
		"../assets/spritesheets/weapons/snazzy_shotgun.bmp", 4.0, 0.6, 6, 10, 59, 30);

	/*
	const Weapon first_weapon = init_weapon("../assets/audio/enemy_sound_test/attack.wav",
		"../assets/spritesheets/weapons/golden_dagger.bmp", 4.0, 0.2, 2, 5, 9, 30);
	*/

	memcpy(player, &first_player, sizeof(Player));
	memcpy(weapon, &first_weapon, sizeof(Weapon));
}

void deinit_all(const Player player, const Weapon weapon) {
	void deinit_weapon(const Weapon);
	void deinit_level(const Level);
	void deinit_screen(void);

	deinit_sound(player.jump.sound_at_jump);
	deinit_sound(player.jump.sound_at_land);
	deinit_weapon(weapon);
	deinit_level(current_level);
	deinit_audio_subsystem();
	deinit_screen();

	TTF_Quit();

	#ifdef TRACK_MEMORY
	dynamic_memory_report();
	#endif

	exit(0);
}
