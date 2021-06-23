inlinable VectorF get_cell_from_ray_pos(const VectorF ray_pos) {
	return (VectorF) {
		(int) floor(fabs(ray_pos[0])) % (current_level.map_width - 1),
		(int) floor(fabs(ray_pos[1])) % (current_level.map_height - 1)
	};
}

inlinable void safe_set_pixbuf_pixel(const int x, const int y, const Uint32 src) {
	if (y >= 0 && y <= settings.screen_height - 1) {
		// if (keys[SDL_SCANCODE_C]) {
			Uint32* dest = get_pixbuf_pixel(x, y); // big slowdown!
			*dest = src;
		// }
	}
}

// handle changes to the FOV
void floorcast(const Player player) {
	const double
		theta = to_radians(player.angle),
		height_over_proj_dist = settings.screen_height / settings.proj_dist;

	const VectorF dir = {cos(theta) / height_over_proj_dist, sin(theta) / height_over_proj_dist};

	const VectorF
		pos = player.pos,
		plane = {-dir[1], dir[0]};

	const VectorF
		width_vec = VectorF_memset(settings.screen_width),
		ray_dir_begin = VectorFF_sub(dir, plane),
		ray_dir_diff = VectorFF_mul(plane, VectorF_memset(2.0));

	// y change is in screen space
	const double y_change = player.pace.screen_offset + player.z_pitch;
	const double
		abs_y_change = fabs(y_change),
		height = player.jump.height;
		// height = current_level.max_point_height - 1.0 - player.jump.height;

	const double pos_z = 0.5 + height * settings.screen_height / settings.proj_dist;
	const double screen_pos_z = pos_z * settings.screen_height;

	for (int y = -abs_y_change; y < settings.half_screen_height + abs_y_change - 1; y++) {
		if (y == settings.half_screen_height) break;

		const int row = abs(y - settings.half_screen_height);
		const VectorF row_dist = VectorF_memset(screen_pos_z / row);
		const VectorF step = VectorFF_div(VectorFF_mul(row_dist, ray_dir_diff), width_vec);
		VectorF ray_pos = VectorFF_add(VectorFF_mul(ray_dir_begin, row_dist), pos);

		const int
			ceiling_y = y + y_change,
			floor_y = settings.screen_height - y + y_change;

		for (int x = 0; x < settings.screen_width; x++) {
			const VectorF cell = get_cell_from_ray_pos(ray_pos);
			const byte point = map_point(current_level.floor_data, cell[0], cell[1]);
			const SDL_Surface* const surface = current_level.walls[point - 1].surface;
			const VectorF tex_w = VectorF_memset(surface -> w);
			const int max_offset = tex_w[0] - 1;

			const VectorF offset = VectorFF_mul(tex_w, VectorFF_sub(ray_pos, cell));
			const Uint32 src = get_surface_pixel(
				surface -> pixels, surface -> pitch,
				(int) offset[0] & max_offset,
				(int) offset[1] & max_offset);

			// src = shade_ARGB_pixel(src, actual_dist, offset);

			safe_set_pixbuf_pixel(x, ceiling_y, src);
			safe_set_pixbuf_pixel(x, floor_y, src);
			ray_pos = VectorFF_add(ray_pos, step);
		}
	}
}

/////

typedef struct {
	enum {
		BeginFloorCast, CurrentlyFloorCasting,
		WaitToNextFloorCast, EndFloorCast
	} state;

	pthread_t thread;
	void* player;
} FloorCastThread;

void* threaded_floorcast(void* const untyped_floorcast_thread) {
	FloorCastThread* thread = (FloorCastThread*) untyped_floorcast_thread;
	Player* player = thread -> player;

	while (thread -> state != EndFloorCast) {
		printf("thread 2, state = %d\n", thread -> state);

		const Uint32 before = SDL_GetTicks();
		if (thread -> state == BeginFloorCast) {
			thread -> state = CurrentlyFloorCasting;
			floorcast(*player);
			thread -> state = WaitToNextFloorCast;
		}
		printf("About to delay intentionally\n");
		tick_delay(before);
		printf("After that delay\n");
	}
	printf("Done!\n");
	return NULL;
}

inlinable FloorCastThread init_floorcast_thread(Player* player) {
	FloorCastThread thread = {.state = WaitToNextFloorCast, .player = player};
	pthread_create(&thread.thread, NULL, threaded_floorcast, (void*) &thread);
	return thread;
}

inlinable void deinit_floorcast_thread(FloorCastThread* const thread) {
	thread -> state = EndFloorCast;
	pthread_join(thread -> thread, NULL);
}

inlinable void start_floorcast_tick(FloorCastThread* const thread) {
	thread -> state = BeginFloorCast;
}

inlinable void wait_for_floorcast_tick(FloorCastThread* const thread) {
	printf("thread 1, state = %d\n", thread -> state);
	while (thread -> state == CurrentlyFloorCasting) {
		printf("Waiting because state = %d\n", CurrentlyFloorCasting);
	}
	thread -> state = WaitToNextFloorCast;
	printf("Just finished floorcast\n");
}
