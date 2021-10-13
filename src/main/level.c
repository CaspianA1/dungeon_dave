/*
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
*/

void init_level(const int map_width, const int map_height,
	const byte* const wallmap, const byte* const heightmap,
	const double init_x, const double init_y, const double init_height,
	const byte max_point_height, const byte out_of_bounds_point,
	const char* const background_sound_path, const char* const skybox_path,
	double (*const shader) (const vec)) {

	#ifndef SOUND_ENABLED
	(void) background_sound_path;
	#endif

	/* The wall, billboard, and animation count are not
	initialized here as constant because for them to be set variadically,
	they need the count variable as the last argument, and specifying it
	twice would be redundant. */

	current_level.map_size = (ivec) {map_width, map_height};
	//////////
	const int bytes = map_width * map_height;

	current_level.wallmap = wmalloc(bytes);
	memcpy(current_level.wallmap, wallmap, bytes);

	current_level.heightmap = wmalloc(bytes);
	memcpy(current_level.heightmap, heightmap, bytes);
	//////////
	current_level.init_pos = (vec) {init_x, init_y};
	current_level.init_height = init_height;

	current_level.max_point_height = max_point_height;
	current_level.out_of_bounds_point = out_of_bounds_point;
	current_level.background_sound = init_sound(background_sound_path, 0);

	current_level.skybox.enabled = skybox_path != NULL;
	if (current_level.skybox.enabled) {
		SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, "1", SDL_HINT_OVERRIDE);
		current_level.skybox.sprite = init_sprite(skybox_path, 0);
		SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, "0", SDL_HINT_OVERRIDE);
	}

	current_level.bfs_visited = init_statemap(map_width, map_height);

	#ifdef SHADING_ENABLED
	current_level.shader = shader;
	Lightmap init_lightmap(void);
	current_level.lightmap = init_lightmap();
	#else
	(void) shader;
	#endif
}

inlinable void fill_level_data(byte* const md, const byte point,
	const int x0, const int x1, const int y0, const int y1, const int width) {

	for (int y = y0; y < y1; y++) {
		for (int x = x0; x < x1; x++) md[y * width + x] = point;
	}
}

// path
void set_level_walls(const unsigned wall_count, ...) {
	current_level.wall_count = wall_count; // unsigned b/c of type promotion semantics
	current_level.walls = wmalloc(wall_count * sizeof(Sprite));

	va_list wall_data;
	va_start(wall_data, wall_count);

	for (byte i = 0; i < wall_count; i++)
		current_level.walls[i] = init_sprite(va_arg(wall_data, const char*), 1);

	va_end(wall_data);
}

// path, x, y, height
void set_level_billboards(const unsigned billboard_count, ...) {
	current_level.billboard_count = billboard_count;
	current_level.billboards = wmalloc(billboard_count * sizeof(Billboard));

	va_list billboard_data;
	va_start(billboard_data, billboard_count);

	for (byte i = 0; i < billboard_count; i++) {
		Billboard* const billboard = current_level.billboards + i; // see if sprite allocated before
		billboard -> sprite = init_sprite(va_arg(billboard_data, const char*), 0);
		billboard -> billboard_data.pos = (vec) {
			va_arg(billboard_data, double),
			va_arg(billboard_data, double)
		};
		billboard -> billboard_data.height = va_arg(billboard_data, double);
	}
	va_end(billboard_data);
}

// from, to
void set_level_teleporters(const unsigned teleporter_count, ...) {
	current_level.teleporter_count = teleporter_count;
	current_level.teleporters = wmalloc(teleporter_count * sizeof(Teleporter));

	va_list teleporter_data;
	va_start(teleporter_data, teleporter_count);

	for (byte i = 0; i < teleporter_count; i++) {
		Teleporter* const teleporter = current_level.teleporters + i;
		DataBillboard* const billboard_data = &teleporter -> from_billboard;

		billboard_data -> pos = (vec) {
			va_arg(teleporter_data, double),
			va_arg(teleporter_data, double)
		};

		billboard_data -> height = *map_point(current_level.heightmap,
			billboard_data -> pos[0], billboard_data -> pos[1]);

		teleporter -> to = (vec) {
			va_arg(teleporter_data, double),
			va_arg(teleporter_data, double)
		};
	}
	va_end(teleporter_data);
}

// x, y, height
void set_level_health_kits(const unsigned health_kit_count, ...) {
	current_level.health_kit_count = health_kit_count;
	current_level.health_kits = wmalloc(health_kit_count * sizeof(HealthKit));

	va_list health_kit_data;
	va_start(health_kit_data, health_kit_count);

	for (byte i = 0; i < health_kit_count; i++)
		current_level.health_kits[i] = (HealthKit) {
			0, {.pos = (vec) {va_arg(health_kit_data, double), va_arg(health_kit_data, double)},
				.height = va_arg(health_kit_data, double)}
		};

	va_end(health_kit_data);
}

// path, frames/row, frames/col, frame_count, fps, x, y, height
void set_level_animated_billboards(const unsigned animated_billboard_count, ...) {
	current_level.animated_billboard_count = animated_billboard_count;
	current_level.animated_billboards = wmalloc(animated_billboard_count * sizeof(AnimatedBillboard));

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
			const int, const int, const int);

		const DataAnimation _animation_data = {
			init_immut_animation_data(path, frames_per_row, frames_per_col, frame_count, fps), {0.0, 0}
		};

		const DataBillboard billboard_data = {
			{va_arg(animation_data, double), va_arg(animation_data, double)},
			.height = va_arg(animation_data, double)
		};

		const AnimatedBillboard animated_billboard = {_animation_data, billboard_data};
		memcpy(current_level.animated_billboards + i, &animated_billboard, sizeof(AnimatedBillboard));
	}
	va_end(animation_data);
}

// enemy index, x, y, height
void set_level_enemy_instances(const unsigned enemy_instance_count, ...) {
	current_level.enemy_instance_count = enemy_instance_count;
	current_level.enemy_instances = wmalloc(enemy_instance_count * sizeof(EnemyInstance));

	va_list enemy_instance_data;
	va_start(enemy_instance_data, enemy_instance_count);

	extern Enemy enemies[enemy_count];

	for (byte i = 0; i < enemy_instance_count; i++) {
		const Enemy* const enemy = enemies + va_arg(enemy_instance_data, unsigned);
		EnemyInstance* const enemy_instance_dest = current_level.enemy_instances + i;

		const EnemyInstance enemy_instance = {
			.enemy = enemy,
			.state = Idle,
			.flags = mask_long_range_attack_enemy * enemy -> is_long_range,
			.hp = enemy -> init_hp,
			.time_at_attack = 0.0,
			.billboard_data = {
				.pos = {va_arg(enemy_instance_data, double), va_arg(enemy_instance_data, double)},
				.beta = 0.0, .dist = 0.0, .height = va_arg(enemy_instance_data, double)
			}
		};

		memcpy(enemy_instance_dest, &enemy_instance, sizeof(EnemyInstance));

		const Navigator nav = init_navigator(current_level.init_pos,
			&enemy_instance_dest -> billboard_data.pos, enemy -> nav_speed,
			enemy_instance.billboard_data.height);

		memcpy(&enemy_instance_dest -> nav, &nav, sizeof(Navigator));
	}
	va_end(enemy_instance_data);
}

inlinable void set_level_thing_container(void) {
	current_level.thing_count =
		current_level.billboard_count + current_level.teleporter_count + current_level.health_kit_count
		+ current_level.animated_billboard_count + current_level.enemy_instance_count;

	current_level.thing_container = wmalloc(current_level.thing_count * sizeof(Thing));
	current_level.max_alloc_thing_count = current_level.thing_count;
}

void deinit_level(void) {
	wfree(current_level.wallmap);
	wfree(current_level.heightmap);

	deinit_statemap(current_level.bfs_visited);

	#ifdef SHADING_ENABLED
	wfree(current_level.lightmap.data);
	#endif

	if (current_level.skybox.enabled) deinit_sprite(current_level.skybox.sprite);
	deinit_sound(&current_level.background_sound);

	for (byte i = 0; i < current_level.wall_count; i++)
		deinit_sprite(current_level.walls[i]);
	wfree(current_level.walls);

	for (byte i = 0; i < current_level.billboard_count; i++)
		deinit_sprite(current_level.billboards[i].sprite);
	wfree(current_level.billboards);

	wfree(current_level.teleporters);
	wfree(current_level.health_kits);

	for (byte i = 0; i < current_level.animated_billboard_count; i++)
		deinit_sprite(current_level.animated_billboards[i].animation_data.immut.sprite);
	wfree(current_level.animated_billboards);

	wfree(current_level.enemy_instances);
	wfree(current_level.thing_container);
}
