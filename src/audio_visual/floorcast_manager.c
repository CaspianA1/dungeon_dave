typedef struct {
	const byte floor_height;
	const vec pos;
	const double p_height;
	const int y_step;
	int y_start;
} FloorcastCallerParams;

void* floorcast_caller(void* const void_fcp) {
	FloorcastCallerParams* const fcp = (FloorcastCallerParams*) void_fcp;
	const int y_start = fcp -> y_start;

	// printf("From %d to %d\n", y_start, y_start + fcp -> y_step);
	fast_affine_floor(fcp -> floor_height, fcp -> pos, fcp -> p_height, y_start, y_start + fcp -> y_step);
	fcp -> y_start += fcp -> y_step; // this may not be updated in the right way?
	pthread_exit(NULL);
}

// expected: total_drawers = 4
void parallel_floorcast(const byte total_drawers, const byte floor_height, const vec pos, const double p_height,
	const int cast_start, const int cast_end) {

	FloorcastCallerParams fcp = {
		.floor_height = floor_height, .pos = pos, .p_height = p_height,
		.y_step = (double) (cast_end - cast_start) / total_drawers, .y_start = cast_start
	};

	// why is the lower part of the screen not drawn?
	pthread_t threads[total_drawers];
	for (byte i = 0; i < total_drawers; i++) pthread_create(threads + i, NULL, floorcast_caller, &fcp);
	for (byte i = 0; i < total_drawers; i++) pthread_join(threads[i], NULL);
}
