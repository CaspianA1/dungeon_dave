inlinable void update_proj_dist(void) {
	settings.proj_dist = settings.half_screen_width / tan(settings.fov / 2.0);

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
	if (new_fov <= 0.0 || new_fov >= M_PI) return;
	settings.fov = new_fov;
	update_proj_dist();
}

inlinable void update_max_fps(const int new_max_fps) {
	if (new_max_fps <= 0) return;
	settings.max_fps = new_max_fps;
	settings.max_delay = 1000.0 / new_max_fps;
}

void init_SDL_framebuffers(const int new_width, const int new_height, const byte should_free) {
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
	settings.half_screen_width = settings.screen_width >> 1;
	settings.half_screen_height = settings.screen_height >> 1;
	settings.avg_dimensions = (INIT_W + INIT_H) >> 1;
	update_max_fps(INIT_MAX_FPS);
	settings.ray_column_width = INIT_RAY_COLUMN_W;

	const double radians_init_fov = to_radians(INIT_FOV);
	update_fov(radians_init_fov);
	settings.init_fov = radians_init_fov;
	settings.fov_step = to_radians(INIT_FOV_STEP);
	settings.max_fov = to_radians(INIT_MAX_FOV);
	settings.minimap_scale = INIT_MINIMAP_SCALE;
}

byte update_screen_dimensions(void) {
	int new_width, new_height;
	SDL_GetWindowSize(screen.window, &new_width, &new_height);

	const byte
		width_not_eq = new_width != settings.screen_width,
		height_not_eq = new_height != settings.screen_height;

	if (width_not_eq || height_not_eq) {
		init_SDL_framebuffers(new_width, new_height, 1);
		settings.avg_dimensions = (new_width + new_height) >> 1;

		deinit_statemap(occluded_by_walls);
		occluded_by_walls = init_statemap(new_width, new_height);

		if (width_not_eq) {
			settings.screen_width = new_width;
			settings.half_screen_width = new_width >> 1;
			update_proj_dist();

			floorcast_val_buffer = wrealloc(floorcast_val_buffer, new_width * sizeof(FloorcastBufferVal));
			depth_buffer = wrealloc(depth_buffer, new_width * sizeof(float));
		}
		if (height_not_eq) {
			settings.screen_height = new_height;
			settings.half_screen_height = new_height >> 1;
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
		.angle = 0.0, .hp = INIT_HP, .is_dead = 0,
		.y_pitch = 0,

		.sound_when_attacked = init_sound("assets/audio/sound_effects/attacked.wav", 1),
		.sound_when_dying = init_sound("assets/audio/sound_effects/dying.wav", 1),

		.jump = {.jumping = 0, .up_v0 = jump_up_v0, .v0 = 0.0,
			.height = init_height, .start_height = init_height,
			.highest_height = init_height, .time_at_jump = 0.0,
			.sound_at_jump = init_sound("assets/audio/sound_effects/jump_up.wav", 1),
			.sound_at_land = init_sound("assets/audio/sound_effects/jump_land.wav", 1)},

		.tilt = {.val = 0.0, .step = tilt_step, .max = tilt_max},

		.pace = {.domain = {.val = 0.0, .step = pace_step, .max = two_pi},
			.offset_scaler = pace_offset_scaler, .screen_offset = 0},

		.body = {.status = 0, .v = 0.0, .max_v_reached = 0.0, .a = body_a, .limit_v = body_limit_v,
			.strafe_v = body_strafe_v, .v_incr_multiplier = body_v_incr_multiplier}
	};
}

void load_all_defaults(void (*load_first_level) (void), Player* const player, Weapon* const weapon) {
	void init_screen(void);
	StateMap init_statemap(const int, const int);
	void init_all_enemies(void);

	void init_teleporter_resources(void);
	void init_health_kit_resources(void);
	void init_gui_resources(void);

	DataAnimationImmut init_immut_animation_data(const char* const, const int, const int, const int, const int);

	const Uint32 before_loading_defaults = SDL_GetTicks();

	STARTUP_LOG("default settings");
	load_default_settings();

	STARTUP_LOG("screen");
	init_screen();

	STARTUP_LOG("sound subsystem");
	init_sound_subsystem();

	STARTUP_LOG("font subsystem");
	if (TTF_Init() == -1) FAIL("Unable to initialize the font library: %s\n", TTF_GetError());

	STARTUP_LOG("gui, health kit, and teleporter resources");
	init_gui_resources();
	init_health_kit_resources();
	init_teleporter_resources();

	srand(time(NULL));
	keys = SDL_GetKeyboardState(NULL);
	floorcast_val_buffer = wmalloc(settings.screen_width * sizeof(FloorcastBufferVal));
	depth_buffer = wmalloc(settings.screen_width * sizeof(float));
	occluded_by_walls = init_statemap(settings.screen_width, settings.screen_height);

	STARTUP_LOG("enemies");
	init_all_enemies();

	STARTUP_LOG("first level");
	load_first_level();

	STARTUP_LOG("player");
	const Player first_player = load_player(4.8, 0.35, 3.0, 0.12, 15.0, 0.08, 0.09, 0.05, 1.9);

	STARTUP_LOG("primary weapon");

	/*
	const Weapon first_weapon = {
		.status = mask_short_range_weapon, .power = 4.0,
		.sound = init_sound("assets/audio/sound_effects/whip_crack.wav", 1),
		.animation_data = {init_immut_animation_data("assets/spritesheets/weapons/whip.bmp", 4, 6, 22, 60), {0.0, 0}}
	};
	*/

	const Weapon first_weapon = {
		.status = mask_paces_sideways_weapon, .power = 10.0,
		.sound = init_sound("assets/audio/sound_effects/shotgun.wav", 1),
		.animation_data = {init_immut_animation_data("assets/spritesheets/weapons/snazzy_shotgun.bmp", 6, 10, 59, 30), {0.0, 0}}
	};

	/*
	const Weapon first_weapon = {
		.status = mask_short_range_weapon | mask_paces_sideways_weapon, .power = 4.0,
		.sound = init_sound("assets/audio/enemy_sounds/eddie_attack.wav", 1),
		.animation_data = {init_immut_animation_data("assets/spritesheets/weapons/golden_dagger.bmp", 2, 5, 9, 22), {0.0, 0}}
	};
	*/

	memcpy(player, &first_player, sizeof(Player));
	memcpy(weapon, &first_weapon, sizeof(Weapon));

	printf("Startup took %u milliseconds\n", SDL_GetTicks() - before_loading_defaults);
}

void deinit_all(const Player* const player, const Weapon* const weapon) {
	void deinit_weapon(const Weapon* const);
	void deinit_level(void);
	void deinit_all_enemies(void);

	void deinit_teleporter_resources(void);
	void deinit_health_kit_resources(void);
	void deinit_gui_resources(void);

	void deinit_screen(void);

	Mix_HaltChannel(-1); // stops all channels before deiniting the associated sounds

	deinit_sound(&player -> sound_when_attacked);
	deinit_sound(&player -> sound_when_dying);
	deinit_sound(&player -> jump.sound_at_jump);
	deinit_sound(&player -> jump.sound_at_land);
	deinit_weapon(weapon);
	deinit_level();

	deinit_all_enemies();

	deinit_teleporter_resources();
	deinit_health_kit_resources();
	deinit_gui_resources();

	deinit_sound_subsystem();
	TTF_Quit();
	deinit_screen();

	wfree(floorcast_val_buffer);
	wfree(depth_buffer);
	deinit_statemap(occluded_by_walls);

	#ifdef TRACK_MEMORY
	dynamic_memory_report();
	#endif

	exit(0);
}
