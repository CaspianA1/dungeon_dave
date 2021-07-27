// a rewrite of render_overlay.c; thing = billboard or animation or enemy

int cmp_things(const void* const a, const void* const b) {
	const double distances[2] = {
		((DataBillboard*) a) -> dist,
		((DataBillboard*) b) -> dist
	};

	if (distances[0] > distances[1]) return -1;
	else if (distances[0] < distances[1]) return 1;
	else return 0;
}

void draw_processed_still_things(DataBillboard*const* const data_billboard_container, const int thing_count) {
	for (byte i = 0; i < thing_count; i++) {
		const DataBillboard* const billboard_data = data_billboard_container[i];
		(void) billboard_data;
	}
}

void draw_still_things(const Player* const player, const double y_shift) {
	(void) y_shift;

	const double player_angle = to_radians(player -> angle);

	const int thing_count = current_level.billboard_count;
	DataBillboard** const data_billboard_container = wmalloc(thing_count * sizeof(DataBillboard*));

	for (byte i = 0; i < thing_count; i++) {
		DataBillboard* const billboard_data = &current_level.billboards[i].billboard_data;

		const vec delta = billboard_data -> pos - player -> pos;
		billboard_data -> beta = atan2(delta[1], delta[0]) - player_angle;
		if (billboard_data -> beta < -two_pi) billboard_data -> beta += two_pi;
		billboard_data -> dist = sqrt(delta[0] * delta[0] + delta[1] * delta[1]);

		data_billboard_container[i] = billboard_data;
	}

	qsort(data_billboard_container, thing_count, sizeof(DataBillboard*), cmp_things);
	draw_processed_still_things(data_billboard_container, thing_count);

	wfree(data_billboard_container);
}
