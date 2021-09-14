inlinable byte get_debug_level_point_height(const byte point, const vec pos) {
	(void) pos;
	if (point == 5 || point == 6) return 1;
	return point;
}

inlinable double debug_level_shader(const vec pos) {
	// return pos[0] / 2.0;

	const vec center = {current_level.map_size.x >> 1, current_level.map_size.y >> 1};
	const Circle center_circle = {center[0], center[1], 0.9};

	const vec center_delta = pos - center;
	return
		fabs(center_delta[0] * center_delta[1]) * 2.0
		+ bloom_circle(pos, center_circle) * 8.0
		+ fabs(tan(pos[0] + pos[1])) * 0.5;
}

void load_debug_level(void) {
	enum {
		map_width = 12, map_height = 10,
		wall_count = 6, billboard_count = 0, teleporter_count = 0,
		animated_billboard_count = 1, enemy_instance_count = 0
	};

	static const byte wall_data[map_height][map_width] = {
		{5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5},
		{5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5},
		{5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5},
		{5, 0, 0, 0, 4, 4, 3, 3, 0, 1, 1, 5},
		{5, 0, 0, 0, 4, 4, 3, 3, 2, 2, 0, 5},
		{5, 0, 2, 0, 4, 0, 3, 3, 0, 0, 0, 5},
		{5, 0, 1, 0, 4, 0, 0, 0, 0, 0, 0, 5},
		{5, 0, 2, 0, 4, 4, 4, 4, 4, 4, 0, 5},
		{5, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 5},
		{5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5}
	};

	Level debug_level = init_level(map_width, map_height, 2.0, 2.0, 0.0);

	set_level_skybox(&debug_level, "assets/skyboxes/red_mountains_2.bmp");
	debug_level.max_point_height = 4; // is 5?
	debug_level.out_of_bounds_point = 1;
	debug_level.background_sound = init_sound("assets/audio/themes/ambient_wind.wav", 0);
	debug_level.get_point_height = get_debug_level_point_height;
	debug_level.shader = debug_level_shader;

	const int bytes = map_width * map_height;
	memcpy(debug_level.wall_data, wall_data, bytes);
	memset(debug_level.ceiling_data, 2, bytes);
	memset(debug_level.floor_data, 2, bytes);

	set_level_walls(&debug_level, wall_count,
		"assets/wolf/mossy.bmp",
		"assets/walls/cobblestone.bmp",
		"assets/wolf/eagle.bmp",
		"assets/wolf/redbrick.bmp",
		"assets/wolf/bluestone.bmp",
		"assets/walls/arthouse_bricks.bmp");

	set_level_billboards(&debug_level, billboard_count);
	set_level_teleporters(&debug_level, teleporter_count);

	set_level_animated_billboards(&debug_level, animated_billboard_count,
		"assets/spritesheets/sonic.bmp", 6, 5, 30, 30, 2.5, 2.5, 0.1);

	memcpy(&current_level, &debug_level, sizeof(Level));
	set_level_enemy_instances(&current_level, enemy_instance_count);
	set_level_thing_container(&current_level);
}
