inlinable Level init_level(const int map_width, const int map_height,
	const double init_x, const double init_y, const double init_height) {

	/* The wall, billboard, and animation count are not
	initialized here as constant because for them to be set variadically,
	they need the count variable as the last argument, and specifying it
	twice would be redundant. */

	Level level = {
		.map_size = {map_width, map_height},
		.init_pos = {init_x, init_y}, .init_height = init_height, .skybox.enabled = 0
	};

	byte** const map_data[3] = {&level.wall_data, &level.ceiling_data, &level.floor_data};
	for (byte i = 0; i < 3; i++)
		*map_data[i] = wmalloc(map_width * map_height);
	return level;
}

void randomize_map(const Level level, byte* const md, const byte* const points, const byte len_points) {
	for (int x = 0; x < current_level.map_size.x; x++) {
		for (int y = 0; y < current_level.map_size.y; y++) {
			md[y * level.map_size.x + x] = points[rand() % len_points];
		}
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
	const Sprite sprite = init_sprite(path);
	level -> skybox.sprite = sprite;
}

// path
void set_level_walls(Level* const level, const unsigned wall_count, ...) {
	level -> wall_count = wall_count; // unsigned b/c of type promotion semantics
	level -> walls = wmalloc(wall_count * sizeof(Sprite));

	va_list wall_data;
	va_start(wall_data, wall_count);

	for (byte i = 0; i < wall_count; i++)
		level -> walls[i] = init_sprite(va_arg(wall_data, const char*));

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
		billboard -> sprite = init_sprite(va_arg(billboard_data, const char*));
		billboard -> pos = (vec) {
			va_arg(billboard_data, double),
			va_arg(billboard_data, double)
		};
		billboard -> height = va_arg(billboard_data, double);
	}
	va_end(billboard_data);
}

// path, frames/row, frames/col, frame_count, fps, x, y, height
void set_level_animations(Level* const level, const unsigned animation_count, ...) {
	level -> animation_count = animation_count;
	level -> animations = wmalloc(animation_count * sizeof(Animation));

	va_list animation_data;
	va_start(animation_data, animation_count);

	for (byte i = 0; i < animation_count; i++) {
		const char* const path = va_arg(animation_data, const char*);

		const int
			frames_per_row = va_arg(animation_data, int),
			frames_per_col = va_arg(animation_data, int),
			frame_count = va_arg(animation_data, int),
			fps = va_arg(animation_data, int);

		const Animation new_animation = init_animation(
			path, frames_per_row, frames_per_col, frame_count, fps);

		Animation* const dest = &level -> animations[i];
		memcpy(dest, &new_animation, sizeof(Animation));
		dest -> billboard.pos = (vec) {
			va_arg(animation_data, double),
			va_arg(animation_data, double)
		};
		dest -> billboard.height = va_arg(animation_data, double);
	}
	va_end(animation_data);
}

/* state, dist_thresholds, hp_to_retreat, hp,
animation_seg_lengths, animations, sounds, navigator */

void set_level_enemies(Level* const level, const unsigned enemy_count, ...) {
	level -> enemy_count = enemy_count;
	level -> enemies = wmalloc(enemy_count * sizeof(Enemy));

	va_list enemy_data;
	va_start(enemy_data, enemy_count);

	for (byte i = 0; i < enemy_count; i++) {
		const EnemyState enemy_state = va_arg(enemy_data, EnemyState);
		Enemy enemy = {
			.dist_wake_from_idle = va_arg(enemy_data, double),
			.hp = va_arg(enemy_data, double),

			.animation_seg_lengths = {
				va_arg(enemy_data, unsigned),
				va_arg(enemy_data, unsigned),
				va_arg(enemy_data, unsigned),
				va_arg(enemy_data, unsigned)
			},

			.sounds = wmalloc(5 * sizeof(Sound))
		};

		const char* animation_path = va_arg(enemy_data, const char*);
		const int
			frames_per_row = va_arg(enemy_data, int), frames_per_col = va_arg(enemy_data, int),
			frame_count = va_arg(enemy_data, int), fps = va_arg(enemy_data, int);
		Animation animations = init_animation(animation_path,
			frames_per_row, frames_per_col, frame_count, fps);
		memcpy(&enemy.animations, &animations, sizeof(Animation));

		Billboard* const billboard = &enemy.animations.billboard;
		billboard -> pos = (vec) {va_arg(enemy_data, double), va_arg(enemy_data, double)};
		billboard -> height = va_arg(enemy_data, double);

		for (byte i = 0; i < 5; i++)
			enemy.sounds[i] = init_sound(va_arg(enemy_data, const char*), 1);

		void set_enemy_state(Enemy*, EnemyState, byte);
		set_enemy_state(&enemy, enemy_state, 1);

		Enemy* const dest = &level -> enemies[i];
		memcpy(dest, &enemy, sizeof(Enemy));

		Billboard* const dest_billboard = &dest -> animations.billboard;
		const Navigator nav = init_navigator(level -> init_pos, &dest_billboard -> pos, va_arg(enemy_data, double));
		memcpy(&dest -> nav, &nav, sizeof(Navigator));

	}
	va_end(enemy_data);
}

inlinable void set_level_generic_billboard_container(Level* const level) {
	level -> generic_billboard_count = level -> billboard_count
		+ level -> animation_count + level -> enemy_count;

	level -> generic_billboards = wmalloc(level -> generic_billboard_count * sizeof(GenericBillboard));
}

void deinit_level(const Level level) {
	byte* const map_data[3] = {level.wall_data, level.ceiling_data, level.floor_data};
	for (byte i = 0; i < 3; i++) wfree(map_data[i]);

	if (level.skybox.enabled) deinit_sprite(level.skybox.sprite);
	deinit_sound(level.background_sound);

	for (byte i = 0; i < level.wall_count; i++)
		deinit_sprite(level.walls[i]);
	wfree(level.walls);

	for (byte i = 0; i < level.billboard_count; i++)
		deinit_sprite(level.billboards[i].sprite);
	wfree(level.billboards);

	for (byte i = 0; i < level.animation_count; i++)
		deinit_sprite(level.animations[i].billboard.sprite);
	wfree(level.animations);

	void deinit_enemy(Enemy);
	for (byte i = 0; i < level.enemy_count; i++)
		deinit_enemy(level.enemies[i]);
	wfree(level.enemies);

	wfree(level.generic_billboards);
}
