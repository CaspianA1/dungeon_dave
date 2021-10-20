DataAnimationImmut
	projectile_traveling_animation,
	projectile_exploding_animation;

static Sound projectile_exploding_sound;

void init_projectile_resources(void) {
	const DataAnimationImmut
		traveling = init_immut_animation_data("assets/spritesheets/fireball_travel.bmp", D_Thing, 12, 1, 12, 15),
		exploding = init_immut_animation_data("assets/spritesheets/fireball_explode.bmp", D_Thing, 8, 1, 8, 15);

	memcpy(&projectile_traveling_animation, &traveling, sizeof(DataAnimationImmut));
	memcpy(&projectile_exploding_animation, &exploding, sizeof(DataAnimationImmut));

	projectile_exploding_sound = init_sound("assets/audio/sound_effects/rocket_explosion.wav", 1);
}

void deinit_projectile_resources(void) {
	deinit_sprite(projectile_traveling_animation.sprite);
	deinit_sprite(projectile_exploding_animation.sprite);
	deinit_sound(&projectile_exploding_sound);
}

inlinable void update_projectile_sound(const Projectile* const projectile) {
	update_channel_from_dist_3D_and_beta(
		projectile -> sound_channel,
		quietest_projectile_sound_dist,
		(double) projectile -> tracer.dist,
		projectile -> billboard_data.beta);
}

inlinable void update_inter_tick_projectiles(const Player* const player, const Weapon* const weapon) {
	byte new_projectile_count = current_level.projectile_count;
	for (byte i = 0; i < current_level.projectile_count; i++) {
		Projectile* const projectile_ref = current_level.projectiles + i;
		const int channel = projectile_ref -> sound_channel;
		const Tracer* const tracer = &projectile_ref -> tracer;

		const BoundingBox_3D projectile_box = init_bounding_box_3D(tracer -> pos, inter_tick_projectile_size);
		const byte collided = apply_damage_from_weapon_if_needed(player, weapon, tracer -> dist, projectile_box);

		if ((projectile_ref -> state == P_Traveling) &&
			(collided || !iter_tracer(&projectile_ref -> tracer) || !channel_still_playing(channel))) {
			projectile_ref -> state = P_Exploding;
			stop_sound_channel(channel);
			projectile_ref -> sound_channel = play_short_sound(&projectile_exploding_sound);
			update_projectile_sound(projectile_ref);
		}
		else if (projectile_ref -> state == P_DoneExploding) {
			current_level.thing_count--;
			new_projectile_count--;
			// Below, all projectiles on the right side of the current element are shifted left by 1,
			// essentially deleting the projectile at position `i`
			memmove(projectile_ref, projectile_ref + 1,
				(current_level.projectile_count - i - 1) * sizeof(Projectile));
		}
		else update_projectile_sound(projectile_ref);
	}
	current_level.projectile_count = new_projectile_count;
}

void use_inter_tick_projectile_weapon(const Player* const player, const int channel) {
	if (current_level.projectile_count == current_level.alloc_projectile_count) {
		current_level.projectiles = wrealloc(current_level.projectiles,
			++current_level.alloc_projectile_count * sizeof(Projectile));

		current_level.thing_container = wrealloc(current_level.thing_container,
			++current_level.alloc_thing_count * sizeof(Thing));
	}

	const Tracer tracer = init_tracer_from_player(player, inter_tick_projectile_tracer_step, 0);

	const Projectile projectile = {
		.billboard_data = {
			.pos = {(double) tracer.pos[0], (double) tracer.pos[1]},
			.height = player -> jump.height
		},
		.state = P_Traveling,
		.sound_channel = channel,
		.tracer = tracer
	};

	memcpy(current_level.projectiles + current_level.projectile_count, &projectile, sizeof(Projectile));

	current_level.thing_count++;
	current_level.projectile_count++;
}
