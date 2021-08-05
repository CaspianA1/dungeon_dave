// a rewrite of render_overlay.c; thing = billboard or animation or enemy

typedef struct {
	const DataBillboard* const billboard_data;
	const Sprite sprite;
} Thing;

static int cmp_things(const void* const a, const void* const b) {
	const double distances[2] = {
		((Thing*) a) -> billboard_data -> dist,
		((Thing*) b) -> billboard_data -> dist
	};

	if (distances[0] > distances[1]) return -1;
	else if (distances[0] < distances[1]) return 1;
	else return 0;
}

static void draw_processed_things(const Player* const player, Thing* const thing_container,
	const int thing_count, const double y_shift) {

	for (byte i = 0; i < thing_count; i++) {
		const Thing thing = thing_container[i];
		const DataBillboard billboard_data = *thing.billboard_data;
		const Sprite sprite = thing.sprite;

		const double
			abs_billboard_beta = fabs(billboard_data.beta),
			cos_billboard_beta = cos(billboard_data.beta);

		if (billboard_data.dist <= 0.01 // if too close
			|| cos_billboard_beta <= 0.0 // if out of view
			|| doubles_eq(abs_billboard_beta, half_pi) // if tan of beta equals inf val for tan
			|| doubles_eq(abs_billboard_beta, three_pi_over_two))
			continue;

		const double corrected_dist = billboard_data.dist * cos_billboard_beta;

		const double
			center_offset = tan(billboard_data.beta) * settings.proj_dist,
			size = settings.proj_dist / corrected_dist;

		const double
			center_x = settings.half_screen_width + center_offset,
			half_size = size / 2.0;

		const double start_x = center_x - half_size;
		if (start_x >= settings.screen_width) continue;

		double end_x = center_x + half_size;
		if (end_x < 0.0) continue;
		else if (end_x > settings.screen_width) end_x = settings.screen_width;

		SDL_Rect src_crop = {
			.y = 0, .w = 1, .h = sprite.size.y
		};

		SDL_FRect screen_pos = {
			0.0, y_shift - half_size
			+ (player -> jump.height - billboard_data.height) * settings.screen_height / corrected_dist,
			settings.ray_column_width, size
		};

		#ifdef SHADING_ENABLED
		const byte shade = 255 * calculate_shade(size, billboard_data.pos);
		SDL_SetTextureColorMod(thing.sprite.texture, shade, shade, shade);
		#endif

		for (int screen_row = start_x; screen_row < end_x; screen_row += settings.ray_column_width) {
			if (screen_row < 0 || (double) val_buffer[screen_row].depth < corrected_dist) continue;
			screen_pos.x = screen_row;
			src_crop.x = ((double) (screen_row - (int) start_x) / size) * sprite.size.x;

			SDL_RenderCopyF(screen.renderer, thing.sprite.texture, &src_crop, &screen_pos);
		}
	}
}

inlinable void update_thing_values(const vec thing_pos, const vec player_pos,
	const double player_angle, double* const beta, double* const dist) {

	const vec delta = thing_pos - player_pos;
	*beta = atan2(delta[1], delta[0]) - player_angle;
	if (*beta < -two_pi) *beta += two_pi;
	*dist = sqrt(delta[0] * delta[0] + delta[1] * delta[1]);
}

static void add_still_things_to_thing_container(Thing* const thing_container, const vec player_pos,
	const double player_angle) {

	for (byte i = 0; i < current_level.billboard_count; i++) {
		Billboard* const billboard = &current_level.billboards[i];
		DataBillboard* const billboard_data = &billboard -> billboard_data;

		update_thing_values(billboard_data -> pos, player_pos, player_angle,
			&billboard_data -> beta, &billboard_data -> dist);

		const Thing thing = {billboard_data, billboard -> sprite};
		memcpy(&thing_container[i], &thing, sizeof(Thing));
	}
}

void draw_things(const Player* const player, const double y_shift) {
	const double player_angle = to_radians(player -> angle);

	const int thing_count = current_level.billboard_count;
	Thing* const thing_container = wmalloc(thing_count * sizeof(Thing));

	add_still_things_to_thing_container(thing_container, player -> pos, player_angle);

	qsort(thing_container, thing_count, sizeof(Thing), cmp_things);
	draw_processed_things(player, thing_container, thing_count, y_shift);

	wfree(thing_container);
}
