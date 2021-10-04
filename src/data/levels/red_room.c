inlinable double red_room_shader(const vec pos) {
	(void) pos;
	return 0.5;
}

void load_red_room(void) {
	enum {
		map_width = 10, map_height = 10,
		wall_count = 2, billboard_count = 0, teleporter_count = 0,
		animated_billboard_count = 0, enemy_instance_count = 0
	};

	static const byte wallmap[map_height][map_width] = {
		{2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
		{2, 0, 2, 0, 0, 0, 0, 0, 0, 2},
		{2, 0, 2, 0, 0, 0, 0, 0, 0, 2},
		{2, 0, 2, 0, 0, 0, 0, 0, 0, 2},
		{2, 0, 2, 0, 0, 0, 0, 0, 0, 2},
		{2, 0, 2, 0, 0, 0, 0, 0, 0, 2},
		{2, 0, 2, 2, 2, 0, 0, 0, 0, 2},
		{2, 0, 2, 2, 2, 0, 0, 0, 0, 2},
		{2, 0, 0, 0, 0, 0, 0, 0, 0, 2},
		{2, 2, 2, 2, 2, 2, 2, 2, 2, 2}
	},

	heightmap[map_height][map_width] = {
		{3, 3, 3, 3, 3, 3, 3, 3, 3, 3},
		{3, 0, 3, 0, 0, 0, 0, 0, 0, 3},
		{3, 0, 3, 0, 0, 0, 0, 0, 0, 3},
		{3, 0, 3, 0, 0, 0, 0, 0, 0, 3},
		{3, 0, 3, 0, 0, 0, 0, 0, 0, 3},
		{3, 0, 3, 0, 0, 0, 0, 0, 0, 3},
		{3, 0, 3, 3, 3, 0, 0, 0, 0, 3},
		{3, 0, 3, 3, 3, 0, 0, 0, 0, 3},
		{3, 0, 0, 0, 0, 0, 0, 0, 0, 3},
		{3, 3, 3, 3, 3, 3, 3, 3, 3, 3},
	};

	init_level(map_width, map_height, (byte*) wallmap, (byte*) heightmap,
		1.5, 1.5, 0.0, 3, 1, "assets/audio/themes/red_room_track.wav", NULL, red_room_shader);

	set_level_walls(wall_count,
		"assets/walls/red_room_floor.bmp",
		"assets/walls/red_curtains.bmp");

	set_level_billboards(billboard_count);
	set_level_teleporters(teleporter_count);
	set_level_animated_billboards(animated_billboard_count);

	set_level_enemy_instances(enemy_instance_count);
	set_level_thing_container();
}
