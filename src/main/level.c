inlinable Level init_level(const int map_width, const int map_height,
	const VectorF init_pos, const double init_height) {

	/* The wall, billboard, and animation count are not
	initialized here as constant because for them to be set variadically,
	they need the count variable as the last argument, and specifying it
	twice would be redundant. */

	Level level = {
		.map_width = map_width, .map_height = map_height,
		.init_pos = init_pos, .init_height = init_height, .skybox.enabled = 0
	};

	byte*** map_data[3] = {&level.wall_data, &level.ceiling_data, &level.floor_data};
	for (byte i = 0; i < 3; i++) {
		byte*** map_datum = map_data[i];
		*map_datum = wmalloc(map_height * sizeof(byte*));

		for (int y = 0; y < map_height; y++)
			(*map_datum)[y] = wmalloc(map_width); // sizeof(byte) = always 1
	}
	return level;
}

void randomize_map(const Level level, byte** map_datum, const byte* points, const byte len_points) {
	for (int y = 0; y < level.map_height; y++) {
		for (int x = 0; x < level.map_width; x++)
			map_datum[y][x] = points[rand() % len_points];
	}
}

inlinable void set_level_skybox(Level* level, const char* path) {
	level -> skybox.enabled = 1;
	const Sprite sprite = init_sprite(path);
	level -> skybox.sprite = sprite;
	level -> skybox.max_width = sprite.surface -> w;
	level -> skybox.max_height = sprite.surface -> h;
}

// path
void set_level_walls(Level* level, const unsigned wall_count, ...) {
	level -> wall_count = wall_count; // unsigned b/c of type promotion semantics
	level -> walls = wmalloc(wall_count * sizeof(Sprite));

	va_list wall_data;
	va_start(wall_data, wall_count);

	for (byte i = 0; i < wall_count; i++)
		level -> walls[i] = init_sprite(va_arg(wall_data, const char*));

	va_end(wall_data);
}

// path, x, y, height
void set_level_billboards(Level* level, const unsigned billboard_count, ...) {
	level -> billboard_count = billboard_count;
	level -> billboards = wmalloc(billboard_count * sizeof(Billboard));

	va_list billboard_data;
	va_start(billboard_data, billboard_count);

	for (byte i = 0; i < billboard_count; i++) {
		Billboard* billboard = &level -> billboards[i];
		billboard -> sprite = init_sprite(va_arg(billboard_data, const char*));
		billboard -> pos[0] = va_arg(billboard_data, double);
		billboard -> pos[1] = va_arg(billboard_data, double);
		billboard -> height = va_arg(billboard_data, double);
	}
	va_end(billboard_data);
}

// path, frames/row, frames/col, frame_count, fps, x, y, height
void set_level_animations(Level* level, const unsigned animation_count, ...) {
	level -> animation_count = animation_count;
	level -> animations = wmalloc(animation_count * sizeof(Animation));

	va_list animation_data;
	va_start(animation_data, animation_count);

	for (byte i = 0; i < animation_count; i++) {
		const char* path = va_arg(animation_data, const char*);
		const int
			frames_per_row = va_arg(animation_data, int),
			frames_per_col = va_arg(animation_data, int),
			frame_count = va_arg(animation_data, int),
			fps = va_arg(animation_data, int);

		const Animation new_animation = init_animation(
			path, frames_per_row, frames_per_col, frame_count, fps);

		Animation* dest = &level -> animations[i];
		memcpy(dest, &new_animation, sizeof(Animation));
		dest -> billboard.pos = (VectorF) {
			va_arg(animation_data, double),
			va_arg(animation_data, double)
		};
		dest -> billboard.height = va_arg(animation_data, double);
	}
	va_end(animation_data);
}

/* state, dist_thresholds, hp_to_retreat, hp,
animation_seg_lengths, animations, sounds, navigator */

void set_level_enemies(Level* level, const unsigned enemy_count, ...) {
	level -> enemy_count = enemy_count;
	level -> enemies = wmalloc(enemy_count * sizeof(Enemy));

	va_list enemy_data;
	va_start(enemy_data, enemy_count);

	for (byte i = 0; i < enemy_count; i++) {
		const EnemyState enemy_state = va_arg(enemy_data, EnemyState);
		Enemy enemy = {
			.dist_thresholds = {
				.begin_attacking = va_arg(enemy_data, double),
				.begin_chasing = va_arg(enemy_data, double),
				.min_idle_sound = va_arg(enemy_data, double),
				.max_idle_sound = va_arg(enemy_data, double),
			},

			.hp_to_retreat = va_arg(enemy_data, double),
			.hp = va_arg(enemy_data, double),

			.animation_seg_lengths = {
				va_arg(enemy_data, unsigned),
				va_arg(enemy_data, unsigned),
				va_arg(enemy_data, unsigned),
				va_arg(enemy_data, unsigned),
				va_arg(enemy_data, unsigned)
			},

			.animations = init_animation(
				va_arg(enemy_data, const char*),
				va_arg(enemy_data, int), va_arg(enemy_data, int),
				va_arg(enemy_data, int), va_arg(enemy_data, int)),

			.sounds = wmalloc(5 * sizeof(Sound))
		};

		void set_enemy_state(Enemy*, EnemyState);
		set_enemy_state(&enemy, enemy_state);

		Billboard* billboard = &enemy.animations.billboard;
		billboard -> pos = (VectorF) {va_arg(enemy_data, double), va_arg(enemy_data, double)};
		billboard -> height = va_arg(enemy_data, double);

		for (byte i = 0; i < 5; i++)
			enemy.sounds[i] = init_sound(va_arg(enemy_data, const char*), 1);

		Enemy* dest = &level -> enemies[i];
		memcpy(dest, &enemy, sizeof(Enemy));

		const Navigator nav = init_navigator(level -> init_pos,
			&dest -> animations.billboard.pos, va_arg(enemy_data, double));

		memcpy(&dest -> navigator, &nav, sizeof(Navigator));

	}
	va_end(enemy_data);
}

inlinable void set_level_generic_billboard_container(Level* level) {
	level -> generic_billboard_count = level -> billboard_count
		+ level -> animation_count + level -> enemy_count;

	level -> generic_billboards = wmalloc(level -> generic_billboard_count * sizeof(GenericBillboard));
}

void deinit_level(const Level level) {
	byte** map_data[3] = {level.wall_data, level.ceiling_data, level.floor_data};
	for (byte i = 0; i < 3; i++) {
		byte** map_datum = map_data[i];
		for (int y = 0; y < level.map_height; y++)
			wfree(map_datum[y]);
		wfree(map_datum);
	}

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
