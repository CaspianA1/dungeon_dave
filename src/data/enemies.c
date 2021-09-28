Enemy init_eddie(void) {
	return (Enemy) {
		.dist_wake_from_idle = 0.9, .power = 3.0, .init_hp = 20.0, .nav_speed = 0.035,
		.short_range = 1, .animation_seg_lengths = {5, 2, 3, 13},
		.animation_data = init_immut_animation_data("assets/spritesheets/eddie.bmp", 23, 1, 23, 12, 0),
		.sounds = {
			init_sound("assets/audio/enemy_sounds/eddie_idle.wav", 1),
			init_sound("assets/audio/enemy_sounds/eddie_chase.wav", 1),
			init_sound("assets/audio/enemy_sounds/eddie_attack.wav", 1),
			init_sound("assets/audio/enemy_sounds/eddie_death.wav", 1),
			init_sound("assets/audio/enemy_sounds/eddie_attacked.wav", 1)
		}
	};
}

Enemy init_trooper(void) {
	return (Enemy) {
		.dist_wake_from_idle = 7.0, .power = 8.0, .init_hp = 30.0, .nav_speed = 0.022,
		.short_range = 1, /* change later */ .animation_seg_lengths = {4, 7, 11, 11},
		.animation_data = init_immut_animation_data("assets/spritesheets/trooper.bmp", 33, 1, 33, 15, 0),

		.sounds = {
			init_sound("assets/audio/enemy_sounds/trooper_idle.wav", 1),
			init_sound("assets/audio/enemy_sounds/eddie_chase.wav", 1),
			init_sound("assets/audio/enemy_sounds/trooper_attack.wav", 1),
			init_sound("assets/audio/enemy_sounds/trooper_attacked.wav", 1),
			init_sound("assets/audio/enemy_sounds/trooper_death.wav", 1)
		}
	};
}

Enemy enemies[enemy_count];
Enemy (*const enemy_loader[enemy_count])(void) = {init_eddie, init_trooper};

void init_all_enemies(void) {
	for (byte i = 0; i < enemy_count; i++) {
		const Enemy enemy = enemy_loader[i]();
		memcpy(enemies + i, &enemy, sizeof(Enemy));
	}
}

void deinit_all_enemies(void) {
	for (byte i = 0; i < enemy_count; i++) deinit_enemy(enemies + i);
}
