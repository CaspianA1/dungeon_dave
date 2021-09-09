typedef struct {
	byte drawer_id;
	const byte floor_height;
	const vec pos;
	const double p_height, pace, y_shift;
	const int y_pitch;
} FloorcastCallerParams;

void* floorcast_caller(void* const void_fcp) {
	FloorcastCallerParams fcp = *(FloorcastCallerParams*) void_fcp;
	(void) fcp;

	pthread_exit(NULL);
}

void manage_floorcast(const byte floor_height, const vec pos,
		const double p_height, const double pace, double y_shift, const int y_pitch) {

	(void) floor_height;
	(void) pos;
	(void) p_height;
	(void) pace;
	(void) y_shift;
	(void) y_pitch;

	const byte drawing_units_in_parallel = 4;

	FloorcastCallerParams fcp = {
		.floor_height = floor_height,
		.pos = pos, .p_height = p_height, .pace = pace, .y_shift = y_shift, .y_pitch = y_pitch
	};

	for (fcp.drawer_id = 0; fcp.drawer_id < drawing_units_in_parallel; fcp.drawer_id++) {
		pthread_t drawing_unit;
		pthread_create(&drawing_unit, NULL, floorcast_caller, &fcp);
	}
}
