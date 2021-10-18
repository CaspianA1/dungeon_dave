void use_hitscan_weapon(const Weapon* const weapon, const Player* const player) {
	const byte short_range_weapon = bit_is_set(weapon -> flags, mask_short_range_weapon);

	Tracer tracer = init_tracer_from_player(player,
		short_range_weapon ? short_range_tracer_step : long_range_tracer_step, 1);

	while (iter_tracer(&tracer)) {
		const BoundingBox_3D projectile_box = init_bounding_box_3D(tracer.pos, hitscan_projectile_size);
		const byte damage_applied = apply_damage_from_weapon_if_needed(player, weapon, tracer.dist, projectile_box);
		if (damage_applied || short_range_weapon) break;
	}
}
