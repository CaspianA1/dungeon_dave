// this is a french castle

inlinable byte get_fleckenstein_point_height(const byte point, const vec pos) {
	(void) pos;
	switch (point) {
		case 1:
			if (pos[1] < 19.0001) return 5;
			else if (pos[1] < 24.0001) return 24.0001 - pos[1];
			else return 5;

		case 2: return 3;

		case 3:
			if (pos[1] < 21.0001) return 9;
			else return 6;

		case 4: return 6;

		default: return point;
	}
}

inlinable double fleckenstein_shader(const vec pos) {
	return pos[0] / 3.0;
}

void load_fleckenstein(void) {
	enum {
		map_width = 30, map_height = 30,
		wall_count = 4, billboard_count = 0, teleporter_count = 1,
		animated_billboard_count = 0, enemy_instance_count = 0
	};

	static const byte wall_data[map_height][map_width] = {
		{2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
		{2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2},
		{2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2},
		{2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2},
		{2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2},
		{2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2},
		{2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2},
		{2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2},
		{2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 2},
		{2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 2},
		{2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 4, 4, 4, 0, 0, 4, 4, 2},
		{2, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 4, 4, 4, 0, 0, 4, 4, 2},
		{2, 0, 0, 0, 3, 1, 1, 1, 1, 3, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 0, 0, 4, 4, 4, 0, 0, 4, 4, 2},
		{2, 0, 0, 0, 3, 3, 3, 3, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 4, 4, 4, 4, 4, 0, 0, 4, 4, 2},
		{2, 0, 0, 0, 3, 1, 1, 3, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 4, 0, 0, 0, 0, 0, 0, 0, 4, 2},
		{2, 0, 0, 0, 3, 1, 1, 3, 1, 3, 3, 0, 0, 3, 3, 3, 3, 0, 0, 0, 3, 4, 4, 0, 4, 0, 4, 0, 4, 2},
		{2, 0, 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 0, 0, 0, 0, 3, 4, 0, 0, 0, 0, 0, 4, 2},
		{2, 0, 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 0, 0, 0, 0, 0, 3, 4, 0, 0, 4, 0, 4, 2},
		{2, 0, 0, 0, 3, 1, 1, 3, 3, 3, 3, 3, 3, 3, 1, 1, 3, 0, 3, 0, 3, 0, 0, 3, 4, 0, 0, 4, 0, 2},
		{2, 0, 0, 0, 3, 3, 3, 3, 0, 0, 0, 0, 0, 3, 1, 1, 3, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 4, 0, 2},
		{2, 0, 0, 0, 3, 3, 3, 3, 0, 0, 0, 0, 0, 3, 1, 1, 3, 0, 0, 0, 0, 3, 4, 4, 4, 4, 0, 0, 0, 2},
		{2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 1, 1, 3, 0, 0, 0, 3, 4, 4, 4, 4, 0, 0, 0, 0, 2},
		{2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 1, 1, 3, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 2},
		{2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 1, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2},
		{2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2},
		{2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 1, 3, 1, 3, 0, 0, 1, 3, 1, 3, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2},
		{2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2},
		{2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2},
		{2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2},
		{2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2}
	};

	Level fleckenstein = init_level(map_width, map_height, 13.0, 27.0, 0.0);

	set_level_skybox(&fleckenstein, "assets/skyboxes/mossy_mountains.bmp");
	fleckenstein.darkest_shade = 0.1;
	fleckenstein.max_point_height = 20;
	fleckenstein.out_of_bounds_point = 1;
	fleckenstein.background_sound = init_sound("assets/audio/themes/ambient_wind.wav", 0);
	fleckenstein.get_point_height = get_fleckenstein_point_height;
	fleckenstein.shader = fleckenstein_shader;

	const int bytes = map_width * map_height;
	memcpy(fleckenstein.wall_data, wall_data, bytes);
	memset(fleckenstein.ceiling_data, 1, bytes);
	memset(fleckenstein.floor_data, 1, bytes);

	set_level_walls(&fleckenstein, wall_count,
		"assets/walls/stone_2.bmp",
		"assets/walls/cobblestone_3.bmp",
		"assets/walls/cobblestone_2.bmp",
		"assets/walls/marble.bmp");

	set_level_billboards(&fleckenstein, billboard_count);
	set_level_teleporters(&fleckenstein, teleporter_count, 9.5, 26.5, 0.0, 15.5, 6.5);

	set_level_animated_billboards(&fleckenstein, animated_billboard_count);

	memcpy(&current_level, &fleckenstein, sizeof(Level));
	set_level_enemy_instances(&current_level, enemy_instance_count);
	set_level_thing_container(&current_level);
}
