byte get_palace_point_height(const byte point, const vec pos) {
	if (point == 1) {
		if (pos[0] <= 8.9999 && pos[1] <= 5.9999) return 3;
		else return 5;
	}

	else if (point == 2) {
		if (pos[1] <= 9.9999) return 1;
		else if (pos[0] >= 21.0001 || pos[1] <= 10.9999) return 2;
		return 3;
	}

	else if (point == 6) {
		if (pos[0] <= 3.9999) return pos[0] + 0.0001;
		else if (pos[0] <= 13.0001) return 3;
		return 2;
	}

	else if (point == 7) {
		if (pos[1] >= 30.9999) return 2;
		return 1;
	}

	switch (point) {
		case 3: return 6;
		case 4: return 3;
		case 5: return 8; // 6 before
		case 8: return 1;
		case 9: return 2;
		default: return point;
	}
}

double palace_shader(const vec pos) {
	static const Circle
		first_health_kit = {5.0, 2.0, 1.0},
		first_downward_stairs = {22.0, 24.0, 5.0},
		torch = {7.5, 12.5, 1.0},
		hidden_area_circle_1 = {2.5, 30.5, 1.5},
		hidden_area_circle_2 = {2.5, 26.5, 1.5},
		hidden_area_circle_3 = {5.0, 22.5, 3.5};

	static const Triangle ravine_jump_entrance = {{15.0001, 24.0}, {15.5, 31.5}, {5.0, 28.0}};

	return
		diffuse_circle(pos, first_health_kit) * 5.0
		+ diffuse_circle(pos, first_downward_stairs) * 0.2
		+ diffuse_circle(pos, torch) * 5.0
		+ diffuse_circle(pos, hidden_area_circle_1) * 0.2
		+ diffuse_circle(pos, hidden_area_circle_2) * 0.4
		+ diffuse_circle(pos, hidden_area_circle_3) * 0.8
		+ flat_triangle(pos, ravine_jump_entrance) * 1.8
		+ 2.0;
}

void load_palace(void) {
	enum {
		map_width = 40, map_height = 40,
		wall_count = 9, billboard_count = 3,
		animation_count = 2, enemy_count = 1
	};

	// in the horse area: a health kit, and the enemy
	static const byte wall_data[map_height][map_width] = {
		{1, 1, 1, 1, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 1, 0, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 1, 0, 3, 5, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 1, 1, 0, 0, 1, 0, 3, 5, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 1, 0, 5, 5, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 1, 0, 5, 5, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 5, 2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 1, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 5, 2, 2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 1, 0, 5, 5, 2, 2, 2, 2, 2, 2, 2, 2, 2, 5, 2, 2, 2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 2, 2, 2, 2, 1, 1, 0, 5, 5, 2, 2, 2, 2, 2, 2, 2, 2, 5, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{3, 2, 2, 2, 2, 1, 1, 0, 3, 5, 2, 2, 2, 2, 2, 2, 2, 5, 2, 2, 2, 2, 5, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{3, 2, 2, 2, 2, 1, 1, 0, 3, 5, 2, 2, 2, 2, 2, 2, 5, 2, 2, 2, 2, 5, 2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{3, 2, 2, 2, 2, 0, 0, 0, 3, 5, 5, 2, 2, 2, 2, 5, 2, 2, 2, 2, 5, 2, 2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{3, 2, 2, 2, 2, 3, 3, 3, 3, 3, 5, 5, 5, 5, 5, 2, 2, 2, 2, 5, 2, 2, 2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 5, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 5, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 5, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 5, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 5, 5, 5, 5, 5, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 9, 9, 9, 9, 9, 9, 9, 9, 3, 5, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 9, 8, 8, 8, 8, 8, 8, 9, 3, 5, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 9, 8, 0, 0, 0, 0, 8, 9, 3, 5, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 9, 8, 0, 0, 0, 0, 8, 9, 3, 5, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 9, 8, 0, 0, 0, 0, 8, 9, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 9, 8, 0, 8, 8, 8, 8, 9, 0, 0, 0, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 6, 6, 6, 6, 6, 6, 6, 6, 6, 0, 0, 6, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 6, 7, 7, 7, 7, 7, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 6, 7, 7, 7, 7, 7, 7, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 6, 7, 7, 6, 6, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 6, 7, 7, 6, 6, 6, 6, 6, 6, 0, 0, 6, 0, 6, 0, 6, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 6, 7, 7, 6, 6, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 6, 7, 7, 6, 2, 2, 2, 2, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 6, 6, 6, 6, 7, 7, 6, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 6, 7, 7, 6, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 6, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 6, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 6, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 6, 6, 6, 6, 6, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
	};

	// previous: {1.1, 1.1, 0.0}, {22.5, 16.0, 5.0}, {2.0, 2.0, 2.0}, {12.0, 37.0, 0.0}, {15.5, 24.5, 2.0}, {2.5. 28.5, 2.0}
	Level palace = init_level(map_width, map_height, 2.5, 28.5, 2.0);
	palace.max_point_height = 8;
	palace.out_of_bounds_point = 1;
	palace.background_sound = init_sound("../assets/audio/themes/sultan.wav", 0);
	palace.get_point_height = get_palace_point_height;
	palace.shader = palace_shader;

	const int bytes = map_width * map_height;
	memcpy(palace.wall_data, wall_data, bytes);
	memset(palace.ceiling_data, 1, bytes);
	memset(palace.floor_data, 1, bytes);

	fill_level_data(palace.floor_data, 5, 4, 6, 1, 3, map_height); // the flying carpet area
	fill_level_data(palace.floor_data, 7, 11, 13, 27, 29, map_height); // the entrance near the obstacle course
	fill_level_data(palace.floor_data, 6, 16, 22, 24, 30, map_height); // the sandstone around the pillars
	fill_level_data(palace.floor_data, 2, 1, 12, 24, 26, map_height); // part 1 of the hidden area's marble
	fill_level_data(palace.floor_data, 2, 1, 9, 19, 26, map_height); // part 2 of the aforementioned
	fill_level_data(palace.floor_data, 2, 1, 4, 25, 39, map_height); // part 3 of the aforementioned

	// "../assets/skyboxes/desert.bmp"
	// "../../Aseprite/Palace City Skybox.bmp"
	set_level_skybox(&palace, "../assets/skyboxes/desert.bmp");

	set_level_walls(&palace, wall_count,
		"../assets/walls/pyramid_bricks_3.bmp",
		"../assets/walls/marble.bmp",
		"../assets/walls/hieroglyphics.bmp",
		"../assets/walls/window.bmp",
		"../assets/walls/rug_1.bmp",
		"../assets/walls/sandstone.bmp",
		"../assets/walls/cobblestone_3.bmp",
		"../assets/walls/horses.bmp",
		"../assets/walls/mesa.bmp");

	set_level_billboards(&palace, billboard_count,
		"../assets/objects/health_kit.bmp", 4.5, 22.5, 0.0, // 11.5, 28.0
		"../assets/objects/hot_dog.bmp", 16.5, 29.5, 0.0,
		"../assets/objects/golden_dome.bmp", 13.0, 28.0, 1.0);

	set_level_animations(&palace, animation_count,
		// "../assets/spritesheets/bogo.bmp", 2, 3, 6, 3, 3.5, 7.0, 0.0,
		"../assets/spritesheets/flying_carpet.bmp", 5, 10, 46, 25, 5.0, 2.0, 0.0,
		"../assets/spritesheets/torch_2.bmp", 2, 3, 5, 12, 7.5, 12.5, 0.0);

	memcpy(&current_level, &palace, sizeof(Level));

	// this is set after b/c it depends on fns that read from current_level
	set_level_enemies(&current_level, enemy_count,
		Idle, // state
		2.0, 8.0, // dist_thresholds
		20.0, // hp
		5, 2, 3, 13, // animation_seg_lengths

		"../assets/spritesheets/eddie.bmp", 23, 1, 23, 12, // animation data
		6.5, 21.5, 0.0, // billboard data (3.0, 7.5 before)

		"../assets/audio/enemy_sound_test/idle.wav", // sound data
		"../assets/audio/enemy_sound_test/chase.wav",
		"../assets/audio/enemy_sound_test/attack.wav",
		"../assets/audio/enemy_sound_test/death.wav",
		"../assets/audio/enemy_sound_test/attacked.wav",

		0.025 /* navigator speed */ );

	set_level_generic_billboard_container(&current_level);
}
