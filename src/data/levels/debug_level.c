inlinable double debug_level_shader(const vec pos) {
	const vec center = {current_level.map_size.x / 2.0, current_level.map_size.y / 2.0};
	const Circle center_circle = {{center[0], center[1]}, 0.9};

	const vec center_delta = pos - center;
	return
		fabs(center_delta[0] * center_delta[1]) * 0.2
		+ bloom_circle(pos, center_circle) * 0.8
		+ fabs(tan(pos[0] + pos[1])) * 0.5;
}

void load_debug_level(void) {
	enum {
		map_width = 12, map_height = 10,
		wall_count = 6, billboard_count = 1, teleporter_count = 0,
		health_kit_count = 0, animated_billboard_count = 1, enemy_instance_count = 0
	};

	static const byte wallmap[map_height][map_width] = {
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
	},

	heightmap[map_height][map_width] = {
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 4, 4, 3, 3, 0, 1, 1, 1},
		{1, 0, 0, 0, 4, 4, 3, 3, 2, 2, 0, 1},
		{1, 0, 2, 0, 4, 0, 3, 3, 0, 0, 0, 1},
		{1, 0, 1, 0, 4, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 2, 0, 4, 4, 4, 4, 4, 4, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
	};

	init_level(map_width, map_height, (byte*) wallmap, (byte*) heightmap,
		2.0, 2.0, 0.0, 4, 1, "assets/audio/themes/ambient_wind.wav",
		"assets/skyboxes/red_mountains_2.bmp", debug_level_shader);

	set_level_walls(wall_count,
		"assets/wolf/mossy.bmp",
		"assets/walls/cobblestone.bmp",
		"assets/wolf/eagle.bmp",
		"assets/wolf/redbrick.bmp",
		"assets/wolf/bluestone.bmp",
		"assets/walls/arthouse_bricks.bmp");

	set_level_billboards(billboard_count, "assets/walls/saqqara.bmp", 6.5, 2.0, 0.0);
	set_level_teleporters(teleporter_count);
	set_level_health_kits(health_kit_count);

	set_level_animated_billboards(animated_billboard_count,
		"assets/spritesheets/sonic.bmp", 6, 5, 30, 30, 2.5, 2.5, 0.1);

	set_level_enemy_instances(enemy_instance_count);
	set_level_thing_container();
}
