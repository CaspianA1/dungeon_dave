byte get_debug_level_point_height(const byte point, const VectorF pos) {
	(void) pos;
	if (point == 5) return 1;
	return point;
}

double debug_level_shader(const VectorF pos) {
	return pos[0] / 2.0;
}

void load_debug_level(void) {
	enum {
		map_width = 12, map_height = 10,
		wall_count = 5, billboard_count = 0,
		animation_count = 1, enemy_count = 0
	};

	static const byte wall_data[map_height][map_width] = {
		{5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5},
		{5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5},
		{5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5},
		{5, 0, 0, 0, 4, 4, 3, 3, 0, 1, 1, 5},
		{5, 0, 0, 0, 4, 4, 3, 3, 2, 2, 0, 5},
		{5, 0, 0, 0, 4, 0, 3, 3, 0, 0, 0, 5},
		{5, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 5},
		{5, 0, 0, 0, 4, 4, 4, 4, 4, 4, 0, 5},
		{5, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 5},
		{5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5}
	};

	Level debug_level = init_level(map_width, map_height, (VectorF) {2.0, 2.0}, 0.0);

	set_level_skybox(&debug_level, "../assets/skyboxes/red_mountains_2.bmp");
	debug_level.max_point_height = 4;
	debug_level.background_sound = init_sound("../assets/audio/ambient_wind.wav", 0);
	debug_level.get_point_height = get_debug_level_point_height;
	debug_level.shader = debug_level_shader;

	for (int y = 0; y < map_height; y++) {
		memcpy(debug_level.wall_data[y], &wall_data[y], map_width);
		memset(debug_level.ceiling_data[y], 2, map_width);
		memset(debug_level.floor_data[y], 2, map_width);
	}

	set_level_walls(&debug_level, wall_count,
		"../assets/wolf/mossy.bmp",
		"../assets/walls/cobblestone.bmp",
		"../assets/wolf/eagle.bmp",
		"../assets/wolf/redbrick.bmp",
		"../assets/wolf/bluestone.bmp");

	set_level_billboards(&debug_level, billboard_count);
	set_level_animations(&debug_level, animation_count,
		"../assets/spritesheets/sonic.bmp", 6, 5, 30, 30, 4.5, 4.5, 0.0);

	memcpy(&current_level, &debug_level, sizeof(Level));

	set_level_enemies(&current_level, enemy_count);

	set_level_generic_billboard_container(&debug_level);
}
