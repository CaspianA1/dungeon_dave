byte get_level_1_point_height(const byte point, const vec pos) {
	switch (point) {
		case 0: return 0;
		case 1: return 3; // cobblestone edges
		case 3: case 5: return 2; // stone, hieroglyphics
		case 4: return (pos[0] < 5.0001) ? 5 : 1;
		default: return 1;
	}
}

double level_1_shader(const vec pos) {
	static const vec
		pillar_center = {4.5, 11.5},
		tunnel_center = {18.0, 7.0},
		plaza_x_range = {16.0, 21.0},
		plaza_y_range = {1.0, 6.0};

	if (vec_in_range(pos[0], plaza_x_range) && vec_in_range(pos[1], plaza_y_range))
		return pos[1] / 2.5; // tan was too expensive for this
		// return fabs(tan(pos[0] - pos[1]));

	const vec pillar_center_diff = vec_diff(pos, pillar_center);
	if (pillar_center_diff[0] < 0.75 && pillar_center_diff[1] < 0.75)
		return pillar_center_diff[0] / pillar_center_diff[1];

	const vec tunnel_center_diff = vec_diff(pos, tunnel_center);
	if (tunnel_center_diff[0] < 3.99 && tunnel_center_diff[1] < 1.99)
		return tunnel_center_diff[0] + tunnel_center_diff[1];

	return 3.5;
}

void load_level_1(void) {
	enum {
		map_width = 25, map_height = 15,
		wall_count = 10, billboard_count = 7,
		animation_count = 3, enemy_count = 0
	};

	// static b/c may be too big for stack
	static const byte wall_data[map_height][map_width] = {
		{4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{4, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{4, 0, 0, 0, 4, 0, 0, 0, 0, 3, 0, 3, 0, 3, 0, 0, 0, 0, 0, 0, 0, 3, 3, 0, 1},
		{4, 0, 0, 0, 4, 0, 0, 3, 0, 3, 0, 3, 0, 3, 0, 3, 0, 0, 0, 0, 0, 3, 3, 0, 1},
		{4, 4, 4, 0, 4, 0, 0, 3, 0, 3, 0, 3, 0, 3, 0, 3, 0, 0, 0, 0, 0, 3, 3, 0, 1},
		{1, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 4, 0, 0, 0, 0, 1},
		{1, 0, 4, 4, 4, 0, 0, 5, 5, 5, 5, 5, 5, 5, 5, 5, 0, 7, 7, 7, 7, 7, 7, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 5, 0, 7, 0, 0, 0, 0, 7, 0, 1},
		{1, 0, 0, 2, 0, 1, 0, 5, 0, 6, 0, 0, 0, 6, 0, 5, 0, 7, 0, 0, 7, 0, 7, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 5, 0, 7, 0, 0, 7, 0, 7, 0, 1},
		{1, 0, 0, 1, 0, 2, 0, 5, 5, 5, 5, 0, 5, 5, 5, 5, 0, 7, 7, 7, 7, 0, 7, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 1},
		{1, 0, 0, 2, 0, 1, 0, 0, 0, 0, 5, 0, 5, 0, 0, 0, 0, 0, 7, 7, 7, 7, 7, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
	};

	// old init positions: {18.5, 9.0}, {3.0, 3.0}, {7.0, 12.0}
	Level level_1 = init_level(map_width, map_height, 17.5, 11.5, 0.0);

	level_1.max_point_height = 5;
	level_1.out_of_bounds_point = 7;
	level_1.background_sound = init_sound("../assets/audio/themes/ambient_wind.wav", 0);
	level_1.get_point_height = get_level_1_point_height;
	level_1.shader = level_1_shader;

	const int bytes = map_width * map_height;
	memcpy(level_1.wall_data, wall_data, bytes);
	memset(level_1.ceiling_data, 4, bytes); // sand
	memset(level_1.floor_data, 4, bytes);

	fill_level_data(level_1.floor_data, 9, 18, 20, 8, 10, map_width); // dirt

	for (byte y = 2; y <= 4; y++) {
		set_map_point(level_1.floor_data, 10, 16, y, map_width); // grass
		set_map_point(level_1.floor_data, 10, 20, y, map_width);
		set_map_point(level_1.floor_data, 10, y + 15, 5, map_width);
		set_map_point(level_1.floor_data, 10, y + 15, 1, map_width);

		for (byte x = 17; x <= 19; x++)
			set_map_point(level_1.floor_data, 8, x, y, map_width); // water
	}

	set_map_point(level_1.floor_data, 10, 16, 5, map_width);
	set_map_point(level_1.floor_data, 10, 16, 1, map_width);
	set_map_point(level_1.floor_data, 10, 20, 1, map_width);
	set_map_point(level_1.floor_data, 10, 20, 5, map_width);

	set_level_skybox(&level_1, "../assets/skyboxes/mossy_mountains_2.bmp");

	set_level_walls(&level_1, wall_count,
		"../assets/walls/cobblestone_2.bmp",
		"../assets/walls/cobblestone_3.bmp",
		"../assets/walls/stone_2.bmp",
		"../assets/walls/pyramid_bricks_3.bmp",
		"../assets/walls/hieroglyphics.bmp",
		"../assets/walls/desert_snake.bmp",
		"../assets/wolf/colorstone.bmp",
		"../assets/walls/water.bmp",
		"../assets/walls/dirt.bmp",
		"../assets/walls/grass.bmp");

	set_level_billboards(&level_1, billboard_count,
		"../assets/objects/tomato.bmp", 6.0, 2.0, 1.0,
		"../assets/objects/demon_head.bmp", 23.5, 1.5, 0.0,
		"../assets/objects/jungle.bmp", 11.5, 11.5, 0.0,
		"../assets/objects/health_kit.bmp", 12.0, 7.5, 0.0,
		"../assets/objects/idol.bmp", 18.5, 3.5, 0.0,
		"../assets/objects/axe.bmp", 11.0, 7.5, 0.0,
		"../assets/wolf/lamp.bmp", 18.5, 11.5, 0.0);

	set_level_animations(&level_1, animation_count,
		"../assets/spritesheets/bogo.bmp", 2, 3, 6, 3, 4.5, 11.5, 0.0,
		"../assets/spritesheets/gold_key.bmp", 3, 4, 12, 6, 1.5, 1.4, 0.0,
		"../assets/spritesheets/torch.bmp", 3, 3, 9, 9, 19.0, 9.5, 0.0);

	memcpy(&current_level, &level_1, sizeof(Level));
	set_level_enemies(&current_level, enemy_count);
	set_level_generic_billboard_container(&current_level);
}
