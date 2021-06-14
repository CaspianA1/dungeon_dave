byte get_temple_level_point_height(const byte point, const VectorF pos) {
	static const VectorI
		stair_1 = {2, 19}, stair_2 = {2, 18},
		stair_3 = {3, 19}, stair_4 = {3, 18};

	if (VectorI_in_cell(pos, stair_1))
		return 1;

	else if (VectorI_in_cell(pos, stair_2)
			|| VectorI_in_cell(pos, stair_3)
			|| VectorI_in_cell(pos, stair_4)) return 2;

	switch (point) {
		case 0: return 0;
		case 1: return 8;
		case 2: return 3;
		case 3: return 7;
		default: return 1;
	}
}

double temple_level_shader(const VectorF pos) {
	static const VectorF sand_tunnel_beginning = {7.5, 8.5};
	static const Circle first_branching_path = {4.0, 26.0, 3.5};
	static const Triangle triangle = {{5.5, 24.5}, {6.0, 15.0}, {15.0, 19.0}};

	const VectorF sand_tunnel_beginning_diff = VectorFF_diff(pos, sand_tunnel_beginning);
	if (sand_tunnel_beginning_diff[0] <= 1.4999 && sand_tunnel_beginning_diff[1] < 2.4999)
		return sand_tunnel_beginning_diff[0] / sand_tunnel_beginning_diff[1] * 0.8;

	return
		diffuse_circle(pos, first_branching_path)
		+ flat_triangle(pos, triangle, 1.5)
		+ 4.0;
}

void load_temple_level(void) {
	enum {
		map_width = 30, map_height = 30,
		wall_count = 4, billboard_count = 1,
		animation_count = 1
	};

	static const byte wall_data[map_height][map_width] = {
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 3, 3, 3, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3, 3, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 3, 2, 2, 2, 2, 3, 3, 3, 2, 2, 2, 2, 3, 3, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 3, 2, 2, 2, 2, 3, 0, 3, 2, 2, 2, 2, 3, 3, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 3, 2, 2, 2, 2, 3, 0, 3, 2, 2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 2, 2, 2, 2, 2, 3, 0, 3, 2, 2, 2, 2, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 2, 2, 3, 3, 3, 3, 3, 0, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 2, 2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 2, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
	};

	Level temple_level = init_level(map_width, map_height, 2.0, 28.0);
	set_level_skybox(&temple_level, "../assets/skyboxes/mossy_mountains.bmp");

	temple_level.max_point_height = 8;
	temple_level.background_sound = init_sound("../assets/audio/storm.wav");
	temple_level.get_point_height = get_temple_level_point_height;
	temple_level.shader = temple_level_shader;

	set_level_walls(&temple_level, wall_count,
		"../assets/walls/hieroglyphics.bmp",
		"../assets/walls/pyramid_bricks_2.bmp",
		"../assets/walls/marble.bmp",
		"../assets/walls/pyramid_bricks_3.bmp");

	set_level_billboards(&temple_level, billboard_count, // 5.5, 9.5, 5.0
		"../assets/objects/ganesha.bmp", 8.0, 25.0, 0.0); // 8.0, 25.0, 3.0

	set_level_animations(&temple_level, animation_count,
		"../assets/spritesheets/tut.bmp", 6, 5, 29, 10, 20.0, 5.0);

	for (int y = 0; y < map_height; y++) {
		memcpy(temple_level.wall_data[y], &wall_data[y], map_width);
		memset(temple_level.ceiling_data[y], 3, map_width);
		memset(temple_level.floor_data[y], 4, map_width);
	}

	memcpy(&current_level, &temple_level, sizeof(Level));
}
