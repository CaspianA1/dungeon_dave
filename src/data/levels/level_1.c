inlinable double level_1_shader(const vec pos) {
	static const vec pillar_center = {4.5, 11.5};
	static const Circle idol_center_circle = {{18.5, 3.5}, 3.0};
	static const Triangle enemy_area_triangle = {{11.0, 10.0}, {12.0, 10.0}, {10.0, 8.0}};

	const vec pillar_center_diff = pos - pillar_center;
	if (fabs(pillar_center_diff[0]) < 0.75 && fabs(pillar_center_diff[1]) < 0.75)
		return fabs(pillar_center_diff[0] / pillar_center_diff[1]);

	return
		fabs(tan(bloom_circle(pos, idol_center_circle)))
		+ flat_triangle(pos, enemy_area_triangle)
		+ 3.5;
}

void load_level_1(void) {
	enum {
		map_width = 25, map_height = 15,
		wall_count = 7, billboard_count = 6, teleporter_count = 0,
		health_kit_count = 1, animated_billboard_count = 3, enemy_instance_count = 1
	};

	// static b/c may be too big for stack
	static const byte wallmap[map_height][map_width] = {
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
	},

	heightmap[map_height][map_width] = {
		{5, 5, 5, 5, 5, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3},
		{5, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3},
		{5, 0, 0, 0, 5, 0, 0, 0, 0, 2, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 3},
		{5, 0, 0, 0, 5, 0, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 0, 0, 0, 0, 2, 2, 0, 3},
		{5, 5, 5, 0, 5, 0, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 0, 0, 0, 0, 2, 2, 0, 3},
		{3, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 2, 1, 0, 0, 0, 0, 3},
		{3, 0, 5, 5, 5, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 1, 1, 1, 1, 1, 0, 3},
		{3, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 1, 0, 0, 0, 0, 1, 0, 3},
		{3, 0, 0, 1, 0, 3, 0, 2, 0, 1, 0, 0, 0, 1, 0, 2, 0, 1, 0, 0, 1, 0, 1, 0, 3},
		{3, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 1, 0, 0, 1, 0, 1, 0, 3},
		{3, 0, 0, 3, 0, 1, 0, 2, 2, 2, 2, 0, 2, 2, 2, 2, 0, 1, 1, 1, 1, 0, 1, 0, 3},
		{3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 3},
		{3, 0, 0, 1, 0, 3, 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 3},
		{3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3},
		{3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3}
	};

	// {18.5, 9.0}, {3.0, 3.0}, {7.0, 12.0}
	init_level(map_width, map_height, (byte*) wallmap, (byte*) heightmap,
		17.5, 11.5, 0.0, 5, 7, "assets/audio/themes/ambient_wind.wav",
		"assets/skyboxes/mossy_mountains_2.bmp", level_1_shader);

	// fill_level_data(level_1.floor_data, 9, 18, 20, 8, 10, map_width); // dirt

	for (byte y = 2; y <= 4; y++) {
		/* set_map_point(level_1.floor_data, 10, 16, y, map_width); // grass
		set_map_point(level_1.floor_data, 10, 20, y, map_width);
		set_map_point(level_1.floor_data, 10, y + 15, 5, map_width);
		set_map_point(level_1.floor_data, 10, y + 15, 1, map_width);

		for (byte x = 17; x <= 19; x++)
			set_map_point(level_1.floor_data, 8, x, y, map_width); // water */
	}

	/*
	set_map_point(level_1.floor_data, 10, 16, 5, map_width);
	set_map_point(level_1.floor_data, 10, 16, 1, map_width);
	set_map_point(level_1.floor_data, 10, 20, 1, map_width);
	set_map_point(level_1.floor_data, 10, 20, 5, map_width);
	*/

	set_level_walls(wall_count,
		"assets/walls/cobblestone_2.bmp",
		"assets/walls/cobblestone_3.bmp",
		"assets/walls/stone_2.bmp",
		"assets/walls/pyramid_bricks_3.bmp",
		"assets/walls/hieroglyphics.bmp",
		"assets/walls/desert_snake.bmp",
		"assets/wolf/colorstone.bmp");

	set_level_billboards(billboard_count,
		"assets/objects/tomato.bmp", 6.0, 2.0, 1.0,
		"assets/objects/demon_head.bmp", 23.5, 1.5, 0.0,
		"assets/objects/jungle.bmp", 11.5, 11.5, 0.0,
		"assets/objects/idol.bmp", 18.5, 3.5, 0.0,
		"assets/objects/axe.bmp", 11.0, 7.5, 0.0,
		"assets/wolf/lamp.bmp", 18.5, 11.5, 0.0);

	set_level_teleporters(teleporter_count);
	set_level_health_kits(health_kit_count, 12.0, 7.5, 0.0);

	set_level_animated_billboards(animated_billboard_count,
		"assets/spritesheets/bogo.bmp", 2, 3, 6, 3, 4.5, 11.5, 0.0,
		"assets/spritesheets/gold_key.bmp", 3, 4, 12, 6, 1.5, 1.4, 0.0,
		"assets/spritesheets/torch.bmp", 3, 3, 9, 9, 19.0, 9.5, 0.0);

	set_level_enemy_instances(enemy_instance_count, 0, 8.5, 7.5, 0.0);
	set_level_thing_container();
}
