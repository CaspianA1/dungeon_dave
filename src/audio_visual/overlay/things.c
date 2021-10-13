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
		if (bit_is_set(thing.flags, mask_skip_rendering_thing)) continue;

		const DataBillboard billboard_data = *thing.billboard_data;
		const double cos_billboard_beta = cos(billboard_data.beta);

		if (billboard_data.dist <= 0.08 // if too close
			|| cos_billboard_beta <= 0.0 // if out of view
			|| doubles_eq(billboard_data.beta, half_pi) // if tan of beta equals inf val for tan
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

		if (start_x >= settings.screen_width || end_x < 0.0) continue; // if projected out of view
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

//////////

void update_billboard_values(DataBillboard* const billboard_data, const vec p_pos, const double p_angle) {
	const vec delta_2D = billboard_data -> pos - p_pos;

	billboard_data -> beta = p_angle - atan2(delta_2D[1], delta_2D[0]);
	if (billboard_data -> beta > two_pi) billboard_data -> beta -= two_pi;

	billboard_data -> dist = sqrt(delta_2D[0] * delta_2D[0] + delta_2D[1] * delta_2D[1]);
}

#define THING_ADDER(name) add_##name##_things_to_thing_container
#define THING_ADDER_SIGNATURE Thing* const thing_buffer_start, const vec p_pos, const double p_angle
#define DEF_THING_ADDER(type) inlinable void THING_ADDER(type)(THING_ADDER_SIGNATURE)

DEF_THING_ADDER(still) {
	for (byte i = 0; i < current_level.billboard_count; i++) {
		Billboard* const billboard = current_level.billboards + i;
		DataBillboard* const billboard_data = &billboard -> billboard_data;

		update_billboard_values(billboard_data, p_pos, p_angle);

		const Sprite* const sprite = &billboard -> sprite;
		const ivec size = sprite -> size;
		const Thing thing = {0, billboard_data, sprite, {0, 0, size.x, size.y}};

		memcpy(thing_buffer_start + i, &thing, sizeof(Thing));
	}
}

DEF_THING_ADDER(teleporter) {
	for (byte i = 0; i < current_level.teleporter_count; i++) {
		Teleporter* const teleporter = current_level.teleporters + i;
		DataBillboard* const billboard_data = &teleporter -> from_billboard;

		update_billboard_values(billboard_data, p_pos, p_angle);

		const Thing thing = {
			mask_can_move_through_thing, billboard_data, &teleporter_sprite,
			{0, 0, teleporter_sprite.size.x, teleporter_sprite.size.y}
		};

		memcpy(thing_buffer_start + i, &thing, sizeof(Thing));
	}
}

DEF_THING_ADDER(health_kit) {
	extern Sprite health_kit_sprite;

	for (byte i = 0; i < current_level.health_kit_count; i++) {
		HealthKit* const health_kit = current_level.health_kits + i;
		DataBillboard* const billboard_data = &health_kit -> billboard;

		update_billboard_values(billboard_data, p_pos, p_angle);

		const Thing thing = {
			(mask_skip_rendering_thing * health_kit -> used) | mask_can_move_through_thing,
			billboard_data, &health_kit_sprite,
			{0, 0, health_kit_sprite.size.x, health_kit_sprite.size.y}
		};

		memcpy(thing_buffer_start + i, &thing, sizeof(Thing));
	}
}

DEF_THING_ADDER(animated) {
	for (byte i = 0; i < current_level.animated_billboard_count; i++) {
		AnimatedBillboard* const animated_billboard = current_level.animated_billboards + i;
		DataBillboard* const billboard_data = &animated_billboard -> billboard_data;
		DataAnimation* const animation_data = &animated_billboard -> animation_data;
		const DataAnimationImmut* const immut_animation_data = &animation_data -> immut;

		update_billboard_values(billboard_data, p_pos, p_angle);

		const Thing thing = {
			0, billboard_data, &immut_animation_data -> sprite,
			rect_from_ivecs(get_spritesheet_frame_origin(&animated_billboard -> animation_data),
				immut_animation_data -> frame_size)
		};

		progress_animation_data_frame_ind(animation_data);
		memcpy(thing_buffer_start + i, &thing, sizeof(Thing));
	}
}

DEF_THING_ADDER(enemy_instance) {
	for (byte i = 0; i < current_level.enemy_instance_count; i++) {
		EnemyInstance* const enemy_instance = current_level.enemy_instances + i;
		DataBillboard* const billboard_data = &enemy_instance -> billboard_data;
		const DataAnimationImmut* const immut_animation_data = &enemy_instance -> enemy -> animation_data;

		update_billboard_values(billboard_data, p_pos, p_angle);

		const DataAnimation animation_data = {*immut_animation_data, enemy_instance -> mut_animation_data};

		const Thing thing = { // if enemy dead, can move through it
			mask_can_move_through_thing * (enemy_instance -> state == Dead),
			billboard_data, &immut_animation_data -> sprite,
			rect_from_ivecs(get_spritesheet_frame_origin(&animation_data), animation_data.immut.frame_size)
		};

		progress_enemy_instance_frame_ind(enemy_instance);
		memcpy(thing_buffer_start + i, &thing, sizeof(Thing));
	}
}

// - projectiles are the only temp thing

/* On the topic of filling the thing buffer just once:
	- It would work for still things and teleporters without any extra hassle
	- Animated things and enemies would have to have their frame origins updated
	- For intra (between-frame) projectiles, they would have to be dynamically removed and added */

void draw_things(const vec p_pos, const double p_angle, const double p_height, const double horizon_line) {
	typedef struct {
		const byte thing_type_count;
		void (*const adder_fn)(THING_ADDER_SIGNATURE);
	} ThingAdder;

	enum {num_thing_adders = 5};

	const ThingAdder thing_adders[num_thing_adders] = {
		{current_level.billboard_count, THING_ADDER(still)},
		{current_level.teleporter_count, THING_ADDER(teleporter)},
		{current_level.health_kit_count, THING_ADDER(health_kit)},
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
