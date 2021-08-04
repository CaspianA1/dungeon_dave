inlinable byte get_red_room_point_height(const byte point, const vec pos) {
	(void) pos;
	return (point == 2) ? 3 : 0;
}

inlinable double red_room_shader(const vec pos) {
	(void) pos;
	return 0.5;
}

void load_red_room(void) {
	enum {
		map_width = 10, map_height = 10,
		wall_count = 2, billboard_count = 0,
		animated_billboard_count = 0, enemy_instance_count = 0
	};

	static const byte wall_data[map_height][map_width] = {
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
	};

	Level red_room = init_level(map_width, map_height, 1.5, 1.5, 0.0);
	red_room.max_point_height = 3;
	red_room.out_of_bounds_point = 1;
	red_room.background_sound = init_sound("assets/audio/themes/red_room_track.wav", 0);
	red_room.get_point_height = get_red_room_point_height;
	red_room.shader = red_room_shader;

	const int bytes = map_width * map_height;
	memcpy(red_room.wall_data, wall_data, bytes);
	memset(red_room.ceiling_data, 1, bytes);
	memset(red_room.floor_data, 1, bytes);

	set_level_walls(&red_room, wall_count,
		"assets/walls/red_room_floor.bmp",
		"assets/walls/red_curtains.bmp");	

	set_level_billboards(&red_room, billboard_count);	
	set_level_animated_billboards(&red_room, animated_billboard_count);	

	memcpy(&current_level, &red_room, sizeof(Level));

	set_level_enemy_instances(&current_level, enemy_instance_count);
	set_level_generic_billboard_container(&current_level);
}
