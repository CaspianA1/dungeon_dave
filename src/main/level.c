void print_heightmap(void) {
	printf("static const byte heightmap[map_height][map_width] = {\n");
	for (int y = 0; y < current_level.map_size.y; y++) {
		printf("\t{");
		for (int x = 0; x < current_level.map_size.x; x++) {
			const byte point = *map_point(current_level.wall_data, x, y);
			const byte point_height = current_level.get_point_height(point, (vec) {x, y});
			const char* const str = (x < current_level.map_size.x - 1) ? "%d, " : "%d},\n";

			printf(str, point_height);
			// set_map_point(current_level.heightmap, current_level.get_point_height(x, y), x, y, current_level.map_size.x);
		}
	}
	puts("};");
}

inlinable Level init_level(const int map_width, const int map_height,
	const double init_x, const double init_y, const double init_height) {

	/* The wall, billboard, and animation count are not
	initialized here as constant because for them to be set variadically,
	they need the count variable as the last argument, and specifying it
	twice would be redundant. */

	Level level = {
		.map_size = {map_width, map_height},
		.init_pos = {init_x, init_y}, .init_height = init_height, .skybox.enabled = 0,
		.bfs_visited = init_statemap(map_width, map_height)
	};

	byte** const map_data[4] = {&level.wall_data, &level.ceiling_data, &level.floor_data, &level.heightmap};
	for (byte i = 0; i < 4; i++) *map_data[i] = wmalloc(map_width * map_height);

	return level;
}

void randomize_map(const Level level, byte* const md, const byte* const points, const byte len_points) {
	for (int x = 0; x < current_level.map_size.x; x++) {
		for (int y = 0; y < current_level.map_size.y; y++)
			md[y * level.map_size.x + x] = points[rand() % len_points];
	}
}

inlinable void fill_level_data(byte* const md, const byte point,
	const int x0, const int x1, const int y0, const int y1, const int width) {

	for (int x = x0; x < x1; x++) {
		for (int y = y0; y < y1; y++)
			md[y * width + x] = point;
	}
}

inlinable void set_level_skybox(Level* const level, const char* const path) {
	level -> skybox.enabled = 1;
	const Sprite sprite = init_sprite(path, 0);
	level -> skybox.sprite = sprite;
}

// path
void set_level_walls(Level* const level, const unsigned wall_count, ...) {
	level -> wall_count = wall_count; // unsigned b/c of type promotion semantics
	level -> walls = wmalloc(wall_count * sizeof(Sprite));

	va_list wall_data;
	va_start(wall_data, wall_count);

	for (byte i = 0; i < wall_count; i++)
		level -> walls[i] = init_sprite(va_arg(wall_data, const char*), 1);

	va_end(wall_data);
}

// path, x, y, height
void set_level_billboards(Level* const level, const unsigned billboard_count, ...) {
	level -> billboard_count = billboard_count;
	level -> billboards = wmalloc(billboard_count * sizeof(Billboard));

	va_list billboard_data;
	va_start(billboard_data, billboard_count);

	for (byte i = 0; i < billboard_count; i++) {
		Billboard* const billboard = &level -> billboards[i];
		billboard -> sprite = init_sprite(va_arg(billboard_data, const char*), 0);
		billboard -> billboard_data.pos = (vec) {
			va_arg(billboard_data, double),
			va_arg(billboard_data, double)
		};
		billboard -> billboard_data.height = va_arg(billboard_data, double);
	}
	va_end(billboard_data);
}

// from, height, to
void set_level_teleporters(Level* const level, const unsigned teleporter_count, ...) {
	level -> teleporter_count = teleporter_count;
	level -> teleporters = wmalloc(teleporter_count * sizeof(Teleporter));

	va_list teleporter_data;
	va_start(teleporter_data, teleporter_count);

	for (byte i = 0; i < teleporter_count; i++) {
		Teleporter* const teleporter = &level -> teleporters[i];
		DataBillboard* const billboard_data = &teleporter -> from_billboard;

		billboard_data -> pos = (vec) {
			va_arg(teleporter_data, double),
			va_arg(teleporter_data, double)
		};

		billboard_data -> height = va_arg(teleporter_data, double);
		teleporter -> to = (vec) {
			va_arg(teleporter_data, double),
			va_arg(teleporter_data, double)
		};
	}
}

// path, frames/row, frames/col, frame_count, fps, x, y, height
void set_level_animated_billboards(Level* const level, const unsigned animated_billboard_count, ...) {
	level -> animated_billboard_count = animated_billboard_count;
	level -> animated_billboards = wmalloc(animated_billboard_count * sizeof(AnimatedBillboard));

	va_list animation_data;
	va_start(animation_data, animated_billboard_count);

	for (byte i = 0; i < animated_billboard_count; i++) {
		const char* const path = va_arg(animation_data, const char*);

		const int
			frames_per_row = va_arg(animation_data, int),
			frames_per_col = va_arg(animation_data, int),
			frame_count = va_arg(animation_data, int),
			fps = va_arg(animation_data, int);


		DataAnimationImmut init_immut_animation_data(const char* const, const int,
			const int, const int, const int, const byte);

		const DataAnimation _animation_data = {
			init_immut_animation_data(path, frames_per_row, frames_per_col, frame_count, fps, 0), {0.0, 0}
		};

		const DataBillboard billboard_data = {
			{va_arg(animation_data, double), va_arg(animation_data, double)},
			.height = va_arg(animation_data, double)
		};

		const AnimatedBillboard animated_billboard = {_animation_data, billboard_data};
		memcpy(&level -> animated_billboards[i], &animated_billboard, sizeof(AnimatedBillboard));
	}
	va_end(animation_data);
}

// enemy index, x, y, height
void set_level_enemy_instances(Level* const level, const unsigned enemy_instance_count, ...) {
	level -> enemy_instance_count = enemy_instance_count;
	level -> enemy_instances = wmalloc(enemy_instance_count * sizeof(EnemyInstance));

	va_list enemy_instance_data;
	va_start(enemy_instance_data, enemy_instance_count);

	extern Enemy enemies[1];
	for (byte i = 0; i < enemy_instance_count; i++) {
		const byte enemy_ind = va_arg(enemy_instance_data, unsigned);
		Enemy* const enemy = &enemies[enemy_ind];
		EnemyInstance* const enemy_instance_dest = &level -> enemy_instances[i];

		const EnemyInstance enemy_instance = {
			.enemy = enemy,
			.state = Idle,
			.status = 0,
			.hp = enemy -> init_hp,
			.time_at_attack = 0.0,
			.billboard_data = {
				.pos = {va_arg(enemy_instance_data, double), va_arg(enemy_instance_data, double)},
				.beta = 0.0, .dist = 0.0, .height = va_arg(enemy_instance_data, double)
			}
		};

		memcpy(enemy_instance_dest, &enemy_instance, sizeof(EnemyInstance));

		const Navigator nav = init_navigator(level -> init_pos,
			&enemy_instance_dest -> billboard_data.pos, enemy -> nav_speed);

		memcpy(&enemy_instance_dest -> nav, &nav, sizeof(Navigator));
	}
	va_end(enemy_instance_data);
}

inlinable void set_level_thing_container(Level* const level) {
	level -> thing_count = level -> billboard_count + level -> teleporter_count
		+ level -> animated_billboard_count + level -> enemy_instance_count;

	level -> thing_container = wmalloc(level -> thing_count * sizeof(Thing));
}

void deinit_level(const Level* const level) {
	byte* const map_data[4] = {level -> wall_data, level -> ceiling_data, level -> floor_data, level -> heightmap};
	for (byte i = 0; i < 4; i++) wfree(map_data[i]);

	deinit_statemap(level -> bfs_visited);

	#ifdef SHADING_ENABLED
	wfree(level -> lightmap.data);
	#endif

	if (level -> skybox.enabled) deinit_sprite(level -> skybox.sprite);
	deinit_sound(&level -> background_sound);

	for (byte i = 0; i < level -> wall_count; i++)
		deinit_sprite(level -> walls[i]);
	wfree(level -> walls);

	for (byte i = 0; i < level -> billboard_count; i++)
		deinit_sprite(level -> billboards[i].sprite);
	wfree(level -> billboards);

	wfree(level -> teleporters);

	for (byte i = 0; i < level -> animated_billboard_count; i++)
		deinit_sprite(level -> animated_billboards[i].animation_data.immut.sprite);
	wfree(level -> animated_billboards);

	wfree(level -> enemy_instances);
	wfree(level -> thing_container);
}
