// a rewrite of render_overlay.c; thing = billboard or animation or enemy

static int cmp_things(const void* const a, const void* const b) {
	const double distances[2] = {
		((Thing*) a) -> billboard_data -> dist,
		((Thing*) b) -> billboard_data -> dist
	};

	if (distances[0] > distances[1]) return -1;
	else if (distances[0] < distances[1]) return 1;
	else return 0;
}

static void vis_drawing_fn(const double start_x, const int x, SDL_Rect* const thing_crop,
	SDL_FRect* const screen_pos, const double size, SDL_Texture* const texture) {

	screen_pos -> x = start_x;
	screen_pos -> w = x - start_x; // == dest_x_range

	thing_crop -> w *= (double) screen_pos -> w / size; // screen_pos.w / size = thing columns per screen columns
	// what would thing_crop.x be?
	// one last occlusion problem, on the right side of the dirt pillars, where start_x must be greater than 0

	SDL_RenderCopyF(screen.renderer, texture, thing_crop, screen_pos);
}

inlinable void draw_thing_as_cols(SDL_Texture* const texture,
	SDL_Rect* const thing_crop, SDL_FRect* const screen_pos, const double start_x,
	const double end_x, const double corrected_dist, const double size) {

	//////////

	byte in_vis_span = 0;
	int vis_span_start = start_x;

	for (int x = start_x; x < end_x; x++) {
		if (x < 0 || (double) val_buffer[x].depth > corrected_dist) { // yes, span is visible
			if (!in_vis_span) {
				vis_span_start = x;
				in_vis_span = 1;
			}
		}

		else { // no, span is not visible, so draw the last one
			if (in_vis_span) { // draws from vis_span_start to x
				vis_drawing_fn(vis_span_start, x, thing_crop, screen_pos, size, texture);
				in_vis_span = 0;
			}
		}
	}

	// printf("Final vis_drawing_fn call from x = %d to x = %d\n", vis_span_start, (int) end_x);
	if (in_vis_span) vis_drawing_fn(vis_span_start, end_x, thing_crop, screen_pos, size, texture);
}

static void draw_processed_things(const Player* const player, const double y_shift) {
	for (byte i = 0; i < current_level.thing_count; i++) {
		Thing thing = current_level.thing_container[i];
		const DataBillboard billboard_data = *thing.billboard_data;

		const double
			abs_billboard_beta = fabs(billboard_data.beta),
			cos_billboard_beta = cos(billboard_data.beta);

		if (billboard_data.dist <= 0.01 // if too close
			|| cos_billboard_beta <= 0.0 // if out of view
			|| doubles_eq(abs_billboard_beta, half_pi) // if tan of beta equals inf val for tan
			|| doubles_eq(abs_billboard_beta, three_pi_over_two))
			continue;

		const double
			corrected_dist = billboard_data.dist * cos_billboard_beta,
			center_offset = tan(billboard_data.beta) * settings.proj_dist;

		const double
			center_x = settings.half_screen_width + center_offset,
			size = settings.proj_dist / corrected_dist;

		const double half_size = size / 2.0;

		const double start_x = center_x - half_size;
		if (start_x >= settings.screen_width) continue;

		double end_x = center_x + half_size;
		if (end_x < 0.0) continue;
		else if (end_x > settings.screen_width) end_x = settings.screen_width;

		SDL_FRect screen_pos = {
			.y = y_shift - half_size
			+ (player -> jump.height - billboard_data.height) * settings.screen_height / corrected_dist,
			.h = size
		};

		SDL_Texture* const texture = thing.sprite -> texture;

		#ifdef SHADING_ENABLED
		const byte shade = calculate_shade(size, billboard_data.pos);
		SDL_SetTextureColorMod(texture, shade, shade, shade);
		#endif

		draw_thing_as_cols(texture, &thing.src_crop, &screen_pos, start_x, end_x, corrected_dist, size);
	}
}

//////////

void update_thing_values(const vec thing_pos, const vec p_pos, // p_pos = player_pos
	const double p_angle, double* const beta, double* const dist) {

	const vec delta = thing_pos - p_pos;
	*beta = atan2(delta[1], delta[0]) - p_angle;
	if (*beta < -two_pi) *beta += two_pi;
	*dist = sqrt(delta[0] * delta[0] + delta[1] * delta[1]);
}

#define THING_ADDER(name) add_##name##_things_to_thing_container
#define THING_ADDER_SIGNATURE const vec p_pos, const double p_angle
#define DEF_THING_ADDER(type) inlinable void THING_ADDER(type)(THING_ADDER_SIGNATURE)

DEF_THING_ADDER(still) {
	for (byte i = 0; i < current_level.billboard_count; i++) {
		Billboard* const billboard = &current_level.billboards[i];
		DataBillboard* const billboard_data = &billboard -> billboard_data;

		update_thing_values(billboard_data -> pos, p_pos, p_angle,
			&billboard_data -> beta, &billboard_data -> dist);

		const Sprite* const sprite = &billboard -> sprite;
		const ivec size = sprite -> size;
		const Thing thing = {billboard_data, sprite, {0, 0, size.x, size.y}};
		memcpy(&current_level.thing_container[i], &thing, sizeof(Thing));
	}
}

DEF_THING_ADDER(teleporter) {
	for (byte i = 0; i < current_level.teleporter_count; i++) {
		Teleporter* const teleporter = &current_level.teleporters[i];
		DataBillboard* const billboard_data = &teleporter -> from_billboard;

		update_thing_values(billboard_data -> pos, p_pos, p_angle, &billboard_data -> beta, &billboard_data -> dist);

		const Thing thing = {
			billboard_data, &teleporter_sprite, {0, 0, teleporter_sprite.size.x, teleporter_sprite.size.y}
		};

		memcpy(&current_level.thing_container[i + current_level.billboard_count], &thing, sizeof(Thing));
	}
}

DEF_THING_ADDER(animated) {
	for (byte i = 0; i < current_level.animated_billboard_count; i++) {
		AnimatedBillboard* const animated_billboard = &current_level.animated_billboards[i];
		DataBillboard* const billboard_data = &animated_billboard -> billboard_data;
		DataAnimation* const animation_data = &animated_billboard -> animation_data;
		const DataAnimationImmut* const immut_animation_data = &animation_data -> immut;

		update_thing_values(billboard_data -> pos, p_pos, p_angle,
			&billboard_data -> beta, &billboard_data -> dist);

		const Thing thing = {
			billboard_data, &immut_animation_data -> sprite,
			rect_from_ivecs(get_spritesheet_frame_origin(&animated_billboard -> animation_data),
				immut_animation_data -> frame_size)
		};

		progress_animation_data_frame_ind(animation_data);

		memcpy(&current_level.thing_container[i + current_level.billboard_count + current_level.teleporter_count], &thing, sizeof(Thing));
	}
}

DEF_THING_ADDER(enemy_instance) {
	for (byte i = 0; i < current_level.enemy_instance_count; i++) {
		EnemyInstance* const enemy_instance = &current_level.enemy_instances[i];
		DataBillboard* const billboard_data = &enemy_instance -> billboard_data;
		const DataAnimationImmut* const immut_animation_data = &enemy_instance -> enemy -> animation_data;

		update_thing_values(billboard_data -> pos, p_pos, p_angle,
			&billboard_data -> beta, &billboard_data -> dist);

		const DataAnimation animation_data = {*immut_animation_data, enemy_instance -> mut_animation_data};

		const ivec frame_origin = get_spritesheet_frame_origin(&animation_data);
		progress_enemy_instance_frame_ind(enemy_instance);

		const Thing thing = {
			billboard_data, &immut_animation_data -> sprite,
			rect_from_ivecs(frame_origin, animation_data.immut.frame_size)
		};

		memcpy(&current_level.thing_container
			[i + current_level.billboard_count + current_level.teleporter_count + current_level.animated_billboard_count],
			&thing, sizeof(Thing));
	}
}

void draw_things(const Player* const player, const double y_shift) {
	const double p_angle = to_radians(player -> angle);

	enum {num_thing_adders = 4};
	void (*thing_adders[num_thing_adders])(THING_ADDER_SIGNATURE) = {
		THING_ADDER(still), THING_ADDER(teleporter), THING_ADDER(animated), THING_ADDER(enemy_instance)
	};

	for (byte i = 0; i < num_thing_adders; i++)
		thing_adders[i](player -> pos, p_angle);

	qsort(current_level.thing_container, current_level.thing_count, sizeof(Thing), cmp_things);
	draw_processed_things(player, y_shift);
}
