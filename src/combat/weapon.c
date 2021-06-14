inlinable Weapon init_weapon(const char* sound_path,
	const char* spritesheet_path, const int frames_per_row,
	const int frames_per_col, const int frame_count, const int fps) {

	const Weapon weapon = {0,
		init_sound(sound_path, 1),
		init_animation(spritesheet_path, frames_per_row, frames_per_col, frame_count, fps)};

	return weapon;
}

void deinit_weapon(const Weapon weapon) {
	deinit_sound(weapon.sound);
	deinit_sprite(weapon.animation.billboard.sprite);
}

void use_weapon_if_needed(Weapon* weapon, const Player player, const InputStatus input_status) {
	if (weapon -> in_use && weapon -> animation.frame_ind == 0) weapon -> in_use = 0;
	else if (input_status == BeginAnimatingWeapon) {
		if (!weapon -> in_use) play_sound(weapon -> sound, 0);
		weapon -> in_use = 1;
	}

	// -1 -> cycle frame, 0 -> first frame
	animate_weapon(&weapon -> animation, player.pos, -weapon -> in_use,
		player.z_pitch, player.pace.screen_offset);
}
