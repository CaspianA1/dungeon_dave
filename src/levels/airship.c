byte get_airship_point_height(const byte point, const VectorF pos) {
	(void) pos;
	return point;
}

double airship_shader(const VectorF pos) {
	(void) pos;
	return current_level.base_shade;
}

void load_airship(void) {
	enum {
		map_width = 10, map_height = 20,
		wall_count = 1, billboard_count = 0,
		animation_count = 0, enemy_count = 0
	};

	static const byte wall_data[map_height][map_width] = {
		{0, 0, 1, 1, 1, 1, 1, 1, 0, 0},
		{0, 1, 0, 0, 0, 0, 0, 0, 1, 0},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
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
		{1, 0, 0, 0, 0, 0, 0, 0, 1, 0},
		{1, 1, 1, 1, 1, 1, 1, 1, 0, 0}
	};

	// base shade = 4.5
}
