inlinable double tpt_shader(const vec pos) {
	(void) pos;
	return 1.0;
}

void load_tpt(void) {
	enum {
		map_width = 10, map_height = 10,
		wall_count = 1, billboard_count = 0, teleporter_count = 0,
		animated_billboard_count = 0, enemy_instance_count = 0
	};

	static const byte wall_data[map_height][map_width] = {
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 1, 1, 0, 0, 0, 1},
		{1, 0, 0, 0, 1, 1, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
	},
	
	heightmap[map_height][map_width] = {
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 2, 2, 0, 0, 0, 1},
		{1, 0, 0, 0, 2, 2, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
	};

	init_level(map_width, map_height, (byte*) wall_data, (byte*) heightmap,
		1.5, 1.5, 0.0, 2, 1, "assets/audio/themes/red_room_track.wav", tpt_shader);

	set_level_skybox("assets/skyboxes/red_mountains_2.bmp");

	set_level_walls(wall_count, "assets/walls/cross_blue.bmp");

	set_level_billboards(billboard_count);	
	set_level_teleporters(teleporter_count);
	set_level_animated_billboards(animated_billboard_count);	

	set_level_enemy_instances(enemy_instance_count);
	set_level_thing_container();
}
