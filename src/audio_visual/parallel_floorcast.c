typedef struct {
	const byte floor_height;
	const vec pos;
	const double p_height;
	const int horizon_line;
} ImmutFloorcastCallerParams;

typedef struct {
	const ImmutFloorcastCallerParams* const immut;
	const int start_y, end_y;
} FloorcastCallerParams;

void* floorcast_caller(void* const data) {
	const FloorcastCallerParams params = *(FloorcastCallerParams*) data;
	const ImmutFloorcastCallerParams* const i_params = params.immut;

	fast_affine_floor(i_params -> floor_height, i_params -> horizon_line,
		params.start_y, params.end_y, i_params -> pos, i_params -> p_height);

	pthread_exit(NULL);
}

void parallel_floorcast(const byte floor_height, const vec pos, const double p_height, const int horizon_line) {
	const int cast_start = horizon_line, cast_end = settings.screen_height;
	const int y_step = (double) (cast_end - cast_start) / FLOORCAST_THREADS;

	const ImmutFloorcastCallerParams immut_params = {
		.floor_height = floor_height, .pos = pos, .p_height = p_height, .horizon_line = horizon_line
	};

	pthread_t threads[FLOORCAST_THREADS];
	FloorcastCallerParams params[FLOORCAST_THREADS];

	for (byte i = 0; i < FLOORCAST_THREADS; i++) {
		const int new_start = cast_start + y_step * i;
		const FloorcastCallerParams copy_params = {&immut_params, new_start, new_start + y_step};
		memcpy(params + i, &copy_params, sizeof(FloorcastCallerParams));

		pthread_create(threads + i, NULL, floorcast_caller, params + i);
	}
	for (byte i = 0; i < FLOORCAST_THREADS; i++) pthread_join(threads[i], NULL);
}
