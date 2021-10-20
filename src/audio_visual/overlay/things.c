// A rewrite of render_overlay.c; thing = billboard | teleporter | health kit | projectile | animation | enemy

static int cmp_things(const void* const a, const void* const b) {
	const double distances[2] = {
		((Thing*) a) -> billboard_data -> dist,
		((Thing*) b) -> billboard_data -> dist
	};

	if (distances[0] > distances[1]) return -1;
	else if (distances[0] < distances[1]) return 1;
	else return 0;
}

static void draw_processed_things(const double p_height, const double horizon_line) {
	for (byte i = 0; i < current_level.thing_count; i++) {
		const Thing* const thing_ref = current_level.thing_container + i;
		if (bit_is_set(thing_ref -> flags, mask_skip_rendering_thing)) continue;
		const Thing thing = *thing_ref;

		const DataBillboard billboard_data = *thing.billboard_data;
		const double cos_billboard_beta = cos(billboard_data.beta);

		if (billboard_data.dist <= 0.05 // If too close
			|| cos_billboard_beta <= 0.0 // If out of view
			|| doubles_eq(billboard_data.beta, half_pi) // If tan of beta equals inf val for tan
			|| doubles_eq(billboard_data.beta, three_pi_over_two)) continue;

		const double
			corrected_dist = billboard_data.dist * cos_billboard_beta,
			center_offset = tan(billboard_data.beta) * settings.proj_dist;

		const double
			center_x = settings.half_screen_width - center_offset,
			size = settings.proj_dist / corrected_dist;

		const double half_size = size * 0.5;

		const double start_x = center_x - half_size;
		double end_x = center_x + half_size;

		if (start_x >= settings.screen_width || end_x < 0.0) continue; // If projected out of view
		else if (end_x > settings.screen_width) end_x = settings.screen_width;

		SDL_FRect screen_pos = {
			(start_x < 0.0) ? 0.0 : round(start_x),
			get_projected_y(horizon_line, half_size, size, p_height - billboard_data.height),
			settings.ray_column_width, size
		};

		SDL_Rect src_column = {.y = thing.src_crop.y, .w = 1, .h = thing.src_crop.h};

		SDL_Texture* const texture = thing.sprite -> texture;

		#ifdef SHADING_ENABLED
		const byte shade = shade_at(size, billboard_data.pos);
		SDL_SetTextureColorMod(texture, shade, shade, shade);
		#endif

		const double src_size_over_dest_size = thing.src_crop.w / size;

		for (; (double) screen_pos.x < end_x; screen_pos.x += settings.ray_column_width) {
			if ((double) depth_buffer[(long) screen_pos.x] < corrected_dist) continue;

			const int src_offset = ((double) screen_pos.x - start_x) * src_size_over_dest_size;
			src_column.x = src_offset + thing.src_crop.x;

			SDL_RenderCopyF(screen.renderer, texture, &src_column, &screen_pos);
		}
	}
}

void update_billboard_values(DataBillboard* const billboard_data, const vec p_pos, const double p_angle) {
	const vec delta_2D = billboard_data -> pos - p_pos;

	billboard_data -> dist = sqrt(delta_2D[0] * delta_2D[0] + delta_2D[1] * delta_2D[1]);

	billboard_data -> beta = p_angle - atan2(delta_2D[1], delta_2D[0]);
	if (billboard_data -> beta > two_pi) billboard_data -> beta -= two_pi;
}

void draw_things(const vec p_pos, const double p_angle, const double p_height, const double horizon_line) {
	typedef struct {
		const byte thing_type_count;
		void (*const adder_fn)(THING_ADDER_SIGNATURE);
	} ThingAdder;

	enum {num_thing_adders = 6};

	const ThingAdder thing_adders[num_thing_adders] = {
		{current_level.billboard_count, THING_ADDER(still)},
		{current_level.teleporter_count, THING_ADDER(teleporter)},
		{current_level.health_kit_count, THING_ADDER(health_kit)},
		{current_level.projectile_count, THING_ADDER(projectile)},
		{current_level.animated_billboard_count, THING_ADDER(animated)},
		{current_level.enemy_instance_count, THING_ADDER(enemy_instance)}
	};

	Thing* thing_buffer_start = current_level.thing_container;
	for (byte i = 0; i < num_thing_adders; i++) {
		const ThingAdder thing_adder = thing_adders[i];
		thing_adder.adder_fn(thing_buffer_start, p_pos, p_angle);
		thing_buffer_start += thing_adder.thing_type_count;
	}

	qsort(current_level.thing_container, current_level.thing_count, sizeof(Thing), cmp_things);
	draw_processed_things(p_height, horizon_line);
}
