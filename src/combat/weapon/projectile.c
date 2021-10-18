Sprite projectile_sprite;

void init_projectile_resources(void) {
	projectile_sprite = init_sprite("assets/objects/fireball.bmp", 0);
}

void deinit_projectile_resources(void) {
	deinit_sprite(projectile_sprite);
}

inlinable void update_inter_tick_projectiles(const Player* const player, const Weapon* const weapon) {
	byte new_projectile_count = current_level.projectile_count;
	for (byte i = 0; i < current_level.projectile_count; i++) {
		Projectile* const projectile_ref = current_level.projectiles + i;
		const int channel = projectile_ref -> sound_channel;
		const Tracer* const tracer = &projectile_ref -> tracer;

		const BoundingBox_3D projectile_box = init_bounding_box_3D(tracer -> pos, inter_tick_projectile_size);
		const byte collided = apply_damage_from_weapon_if_needed(player, weapon, tracer -> dist, projectile_box);

		if (collided || !iter_tracer(&projectile_ref -> tracer) || !channel_still_playing(channel)) {
			current_level.thing_count--;
			new_projectile_count--;
			/* Below, all projectiles on the right side of the current element are shifted left by 1,
			essentially deleting the projectile at position `i` */
			memmove(projectile_ref, projectile_ref + 1,
				(current_level.projectile_count - i - 1) * sizeof(Projectile));

			// Do a boom noise here; and if hit an enemy instance, apply damage

			stop_sound_channel(channel);
		}
		else update_channel_from_dist_3D_and_beta(
			channel, quietest_projectile_sound_dist,
			(double) projectile_ref -> tracer.dist,
			projectile_ref -> billboard_data.beta);
	}
	current_level.projectile_count = new_projectile_count;
}

void use_inter_tick_projectile_weapon(const Weapon* const weapon, const Player* const player, const int channel) {
	(void) weapon;

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
		.tracer = tracer,
		.sound_channel = channel
	};

	memcpy(current_level.projectiles + current_level.projectile_count, &projectile, sizeof(Projectile));

	current_level.thing_count++;
	current_level.projectile_count++;
}
