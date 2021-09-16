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

static void draw_processed_things(const double p_height, const double horizon_line) {
	for (byte i = 0; i < current_level.thing_count; i++) {
		const Thing thing = current_level.thing_container[i];
		const DataBillboard billboard_data = *thing.billboard_data;

		const double
			abs_billboard_beta = fabs(billboard_data.beta),
			cos_billboard_beta = cos(billboard_data.beta);

		if (billboard_data.dist <= 0.08 // if too close
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

		const double half_size = size * 0.5;

		const double start_x = center_x - half_size;
		double end_x = center_x + half_size;

		if (start_x >= settings.screen_width || end_x < 0.0) continue; // if projected out of view
		else if (end_x > settings.screen_width) end_x = settings.screen_width;

		SDL_FRect screen_pos = {
			round(start_x < 0.0 ? 0.0 : start_x),
			get_projected_y(horizon_line, half_size, size, p_height - billboard_data.height),
			settings.ray_column_width, size
		};

		SDL_Rect src_column = {.y = thing.src_crop.y, .w = 1, .h = thing.src_crop.h};

		SDL_Texture* const texture = thing.sprite -> texture;

		#ifdef SHADING_ENABLED
		const byte shade = calculate_shade(size, billboard_data.pos);
		SDL_SetTextureColorMod(texture, shade, shade, shade);
		#endif

		//////////
		if (settings.half_screen_height >= screen_pos.y && screen_pos.y + screen_pos.h >= settings.half_screen_height)
			SDL_SetTextureColorMod(texture, 220, 20, 60);
		else
			SDL_SetTextureColorMod(texture, 255, 255, 255);
		//////////

		const double src_size_over_dest_size = thing.src_crop.w / size;

		for (; (double) screen_pos.x < end_x; screen_pos.x += settings.ray_column_width) {
			if ((double) depth_buffer[(long) screen_pos.x] < corrected_dist) continue;

			const int src_offset = ((double) screen_pos.x - start_x) * src_size_over_dest_size;
			src_column.x = src_offset + thing.src_crop.x;

			SDL_RenderCopyF(screen.renderer, texture, &src_column, &screen_pos);
		}
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
		const Thing thing = {billboard_data, sprite, {0, 0, size.x, size.y}, NULL};
		memcpy(&current_level.thing_container[i], &thing, sizeof(Thing));
	}
}

DEF_THING_ADDER(teleporter) {
	for (byte i = 0; i < current_level.teleporter_count; i++) {
		Teleporter* const teleporter = &current_level.teleporters[i];
		DataBillboard* const billboard_data = &teleporter -> from_billboard;

		update_thing_values(billboard_data -> pos, p_pos, p_angle, &billboard_data -> beta, &billboard_data -> dist);

		const Thing thing = {
			billboard_data, &teleporter_sprite, {0, 0, teleporter_sprite.size.x, teleporter_sprite.size.y}, NULL
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
				immut_animation_data -> frame_size), NULL
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

		// the enemy instance ptr is cast with the explicit struct b/c EnemyInstance is a forward declaration in overlay.h

		const Thing thing = {
			billboard_data, &immut_animation_data -> sprite,
			rect_from_ivecs(frame_origin, animation_data.immut.frame_size), (struct EnemyInstance *const) enemy_instance
		};

		memcpy(&current_level.thing_container
			[i + current_level.billboard_count + current_level.teleporter_count + current_level.animated_billboard_count],
			&thing, sizeof(Thing));
	}
}

void draw_things(const vec p_pos, const double p_angle, const double p_height, const double horizon_line) {
	enum {num_thing_adders = 4};
	void (*const thing_adders[num_thing_adders])(THING_ADDER_SIGNATURE) = {
		THING_ADDER(still), THING_ADDER(teleporter), THING_ADDER(animated), THING_ADDER(enemy_instance)
	};

	for (byte i = 0; i < num_thing_adders; i++) thing_adders[i](p_pos, p_angle);

	qsort(current_level.thing_container, current_level.thing_count, sizeof(Thing), cmp_things);
	draw_processed_things(p_height, horizon_line);
}
