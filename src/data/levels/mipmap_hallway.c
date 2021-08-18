inlinable byte get_hallway_point_height(const byte point, const vec pos) {
	(void) pos;
	return point;
}

inlinable double hallway_shader(const vec pos) {
	// return (pos[0] + pos[1]) / current_level.map_size.y;
	return pos[1] / 3.0;
}

void load_hallway(void) {
	enum {
		map_width = 10, map_height = 25,
		wall_count = 2, billboard_count = 0, teleporter_count = 3,
		animated_billboard_count = 0, enemy_instance_count = 1
	};

	static const byte wall_data[map_height][map_width] = {
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 2, 2, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
	};

	Level hallway = init_level(map_width, map_height, 5.0, 3.4, 0.0);
	hallway.darkest_shade = 0.4;
	hallway.max_point_height = 2;
	hallway.out_of_bounds_point = 1;
	hallway.background_sound = init_sound("assets/audio/themes/ambient_wind.wav", 0);
	hallway.get_point_height = get_hallway_point_height;
	hallway.shader = hallway_shader;

	const int bytes = map_width * map_height;
	memcpy(hallway.wall_data, wall_data, bytes);
	memset(hallway.ceiling_data, 1, bytes);
	memset(hallway.floor_data, 1, bytes);

	set_level_skybox(&hallway, "assets/skyboxes/desert.bmp");
	set_level_walls(&hallway, wall_count, "assets/walls/arthouse_bricks.bmp", "assets/walls/dirt.bmp");
	set_level_billboards(&hallway, billboard_count);

	set_level_teleporters(&hallway, teleporter_count,
		5.0, 4.0, 0.0, // from pos + height
		5.5, 10.5, /* to pos */

		6.0, 4.0, 0.0, 5.5, 10.5,
		4.0, 4.0, 0.0, 5.5, 4.5);

	set_level_animated_billboards(&hallway, animated_billboard_count);

	memcpy(&current_level, &hallway, sizeof(Level));

	set_level_enemy_instances(&current_level, enemy_instance_count, 0, 5.0, 8.0, 0.0);
	set_level_thing_container(&current_level);
}
