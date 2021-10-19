#define THING_ADDER(name) add_##name##_things_to_thing_container
#define THING_ADDER_SIGNATURE Thing* const thing_buffer_start, const vec p_pos, const double p_angle
#define DEF_THING_ADDER(type) inlinable void THING_ADDER(type)(THING_ADDER_SIGNATURE)

void update_billboard_values(DataBillboard* const, const vec, const double p_angle);

DEF_THING_ADDER(still) {
	for (byte i = 0; i < current_level.billboard_count; i++) {
		Billboard* const billboard = current_level.billboards + i;
		DataBillboard* const billboard_data = &billboard -> billboard_data;

		update_billboard_values(billboard_data, p_pos, p_angle);

		const Sprite* const sprite = &billboard -> sprite;
		const ivec size = sprite -> size;
		const Thing thing = {0, T_Still, billboard_data, sprite, {0, 0, size.x, size.y}};

		memcpy(thing_buffer_start + i, &thing, sizeof(Thing));
	}
}

DEF_THING_ADDER(teleporter) {
	for (byte i = 0; i < current_level.teleporter_count; i++) {
		Teleporter* const teleporter = current_level.teleporters + i;
		DataBillboard* const billboard_data = &teleporter -> from_billboard;

		update_billboard_values(billboard_data, p_pos, p_angle);

		const Thing thing = {
			mask_can_move_through_thing,
			T_Teleporter, billboard_data, &teleporter_sprite,
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
			T_HealthKit,
			billboard_data, &health_kit_sprite,
			{0, 0, health_kit_sprite.size.x, health_kit_sprite.size.y}
		};

		memcpy(thing_buffer_start + i, &thing, sizeof(Thing));
	}
}

DEF_THING_ADDER(projectile) {
	extern DataAnimationImmut projectile_traveling_animation, projectile_exploding_animation;
	for (byte i = 0; i < current_level.projectile_count; i++) {
		Projectile* const projectile = current_level.projectiles + i;
		const vec3D projectile_pos = projectile -> tracer.pos;

		DataBillboard* const billboard_data = &projectile -> billboard_data;
		update_billboard_values(billboard_data, p_pos, p_angle);

		DataAnimationMut* const mut_animation_data = &projectile -> curr_animation_data;
		const DataAnimationImmut* const immut_animation_data = (projectile -> state == P_Exploding)
			? &projectile_exploding_animation : &projectile_traveling_animation;

		DataAnimation animation_data = {*immut_animation_data, *mut_animation_data};

		billboard_data -> pos = (vec) {(double) projectile_pos[0], (double) projectile_pos[1]};
		billboard_data -> height = (double) projectile_pos[2] - actor_eye_height;

		const Thing thing = {
			mask_can_move_through_thing, T_Projectile, billboard_data, &immut_animation_data -> sprite,
			rect_from_ivecs(get_spritesheet_frame_origin(&animation_data), immut_animation_data -> frame_size)
		};

		// If animation cycle done
		if (progress_animation_data_frame_ind(&animation_data) && projectile -> state == P_Exploding)
			projectile -> state = P_DoneExploding;

		memcpy(mut_animation_data, &animation_data.mut, sizeof(DataAnimationMut));

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
			0, T_Animated, billboard_data, &immut_animation_data -> sprite,
			rect_from_ivecs(get_spritesheet_frame_origin(animation_data), immut_animation_data -> frame_size)
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
			T_EnemyInstance,
			billboard_data, &immut_animation_data -> sprite,
			rect_from_ivecs(get_spritesheet_frame_origin(&animation_data), animation_data.immut.frame_size)
		};

		progress_enemy_instance_frame_ind(enemy_instance);
		memcpy(thing_buffer_start + i, &thing, sizeof(Thing));
	}
}
