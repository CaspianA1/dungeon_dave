inlinable double tpt_shader(const vec pos) {
	static const Triangle
		center_triangle_1 = {{6.5, 15.5}, {3.5, 1.5}, {1.5, 14.5}},
		center_triangle_2 = {{5.5, 2.5}, {1.5, 2.5}, {4.5, 12.5}},
		center_triangle_3 = {{6.5, 3.5}, {1.5, 8.5}, {6.5, 10.5}},
		upper_triangle_1 = {{4.5, 18.5}, {1.5, 18.5}, {3.5, 15.5}};

	return
		flat_triangle(pos, center_triangle_1) * 0.5
		+ flat_triangle(pos, center_triangle_2) * 0.8
		+ flat_triangle(pos, center_triangle_3) * 1.1
		+ flat_triangle(pos, upper_triangle_1) * 0.5
		+ 0.5;
}

void load_tpt(void) {
	enum {
		map_width = 10, map_height = 20,
		wall_count = 2, billboard_count = 1, teleporter_count = 3,
		animated_billboard_count = 0, enemy_instance_count = 9
	};

	static const byte wall_data[map_height][map_width] = {
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 0, 0, 0, 0, 0, 0, 1, 1, 1},
		{1, 0, 0, 0, 0, 0, 0, 1, 1, 1},
		{1, 0, 1, 1, 1, 1, 0, 1, 1, 1},
		{1, 0, 1, 1, 1, 1, 0, 1, 1, 1},
		{1, 0, 1, 1, 1, 0, 0, 1, 1, 1},
		{1, 1, 1, 1, 1, 0, 0, 1, 1, 1},
		{1, 1, 1, 1, 1, 0, 0, 1, 1, 1},
		{1, 1, 0, 0, 0, 0, 0, 1, 1, 1},
		{1, 1, 1, 0, 0, 0, 0, 1, 1, 1},
		{1, 1, 1, 0, 0, 0, 0, 1, 1, 1},
		{1, 1, 1, 0, 0, 0, 0, 1, 1, 1},
		{1, 1, 1, 1, 1, 1, 0, 1, 1, 1},
		{2, 1, 1, 1, 1, 1, 0, 1, 1, 1},
		{2, 1, 1, 1, 1, 1, 0, 1, 1, 1},
		{1, 1, 1, 1, 1, 1, 0, 1, 1, 1},
		{1, 1, 1, 1, 1, 1, 0, 1, 1, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
	},
	
	heightmap[map_height][map_width] = {
		{6, 6, 6, 6, 6, 6, 6, 6, 6, 6},
		{6, 0, 0, 0, 0, 0, 0, 6, 2, 6},
		{6, 0, 0, 0, 0, 0, 0, 6, 2, 6},
		{6, 0, 6, 6, 6, 6, 0, 6, 2, 6},
		{6, 0, 4, 3, 3, 6, 0, 6, 2, 6},
		{6, 0, 4, 4, 4, 0, 0, 6, 2, 6},
		{6, 4, 4, 4, 4, 0, 0, 6, 2, 6},
		{6, 4, 4, 4, 4, 0, 0, 6, 2, 6},
		{6, 4, 0, 0, 0, 0, 0, 6, 2, 6},
		{6, 4, 4, 0, 0, 0, 0, 6, 2, 6},
		{6, 4, 4, 0, 0, 0, 0, 6, 2, 6},
		{6, 4, 4, 0, 0, 0, 0, 6, 2, 6},
		{6, 4, 6, 6, 6, 6, 0, 6, 2, 6},
		{6, 3, 3, 3, 3, 6, 0, 6, 2, 6},
		{6, 3, 3, 3, 3, 6, 0, 6, 2, 6},
		{6, 6, 6, 6, 6, 6, 0, 6, 2, 6},
		{6, 3, 3, 3, 3, 6, 0, 6, 2, 6},
		{6, 3, 3, 3, 3, 6, 6, 6, 2, 6},
		{6, 3, 2, 2, 2, 2, 2, 2, 2, 6},
		{6, 6, 6, 6, 6, 6, 6, 6, 6, 6}
	};

	init_level(map_width, map_height, (byte*) wall_data, (byte*) heightmap,
		1.5, 1.5, 0.0, 6, 1, "assets/audio/themes/dream_fractal.wav",
		"assets/skyboxes/night.bmp", tpt_shader);

	set_level_walls(wall_count,
		"assets/walls/pyramid_bricks_2.bmp",
		"assets/walls/window.bmp");

	set_level_billboards(billboard_count, "assets/objects/health_kit.bmp", 7.5, 2.5, 6.0);

	set_level_teleporters(teleporter_count,
		6.5, 16.5, 0.0, 8.5, 1.5,
		3.5, 16.5, 3.0, 3.5, 13.5,
		4.5, 4.5, 3.0, 4.5, 3.5);

	set_level_animated_billboards(animated_billboard_count);	

	set_level_enemy_instances(enemy_instance_count,
		1, 2.5, 8.5, 0.0,
		0, 4.5, 8.5, 0.0,
		1, 6.5, 8.5, 0.0,
		0, 6.5, 14.5, 0.0,
		1, 8.5, 5.5, 2.0,
		1, 8.5, 14.5, 2.0,
		0, 1.5, 17.5, 3.0,
		0, 4.5, 16.5, 3.0,
		1, 1.5, 8.5, 4.0);

	set_level_thing_container();
}
