typedef struct {
	const byte floor_height;
	const vec pos;
	const double p_height;
} ImmutFloorcastCallerParams;

typedef struct {
	const ImmutFloorcastCallerParams* const immut;
	const int y_start, y_end;
} FloorcastCallerParams;

void* floorcast_caller(void* const data) {
	const FloorcastCallerParams params = *(FloorcastCallerParams*) data;
	const ImmutFloorcastCallerParams* const i_params = params.immut;

	fast_affine_floor(i_params -> floor_height, params.y_start, params.y_end, i_params -> pos, i_params -> p_height);
	pthread_exit(NULL);
}

// expected: total_drawers = 4
void parallel_floorcast(const byte total_drawers, const byte floor_height, const vec pos, const double p_height,
	const int cast_start, const int cast_end) {

	const int y_step = (double) (cast_end - cast_start) / total_drawers;

	const ImmutFloorcastCallerParams immut_params = {
		.floor_height = floor_height, .pos = pos, .p_height = p_height,
	};

	// why is the lower part of the screen not drawn?
	pthread_t threads[total_drawers];
	for (byte i = 0; i < total_drawers; i++) {
		const int new_start = cast_start + y_step * i;
		FloorcastCallerParams params = {&immut_params, new_start, new_start + y_step};

		// printf("For drawer #%d, draw from %d to %d\n", i + 1, params.y_start, params.y_end);
		pthread_create(threads + i, NULL, floorcast_caller, &params);
	}
	// puts("---");

	for (byte i = 0; i < total_drawers; i++) pthread_join(threads[i], NULL);
}
