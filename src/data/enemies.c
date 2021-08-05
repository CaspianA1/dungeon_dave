Enemy init_eddie(void) {
	return (Enemy) {
		.dist_wake_from_idle = 0.9, .power = 3.0, .init_hp = 20.0, .nav_speed = 0.035,
		.animation_seg_lengths = {5, 2, 3, 13},
		.animation_data = init_immut_animation_data("assets/spritesheets/eddie.bmp", 23, 1, 23, 12, 0),
		.sounds = {
			init_sound("assets/audio/enemy_sound_test/idle.wav", 1),
			init_sound("assets/audio/enemy_sound_test/chase.wav", 1),
			init_sound("assets/audio/enemy_sound_test/attack.wav", 1),
			init_sound("assets/audio/enemy_sound_test/death.wav", 1),
			init_sound("assets/audio/enemy_sound_test/attacked.wav", 1)
		}
	};
}

Enemy enemies[1];
