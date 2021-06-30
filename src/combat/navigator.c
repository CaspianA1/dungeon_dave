// need actual wall collision detection for viable good-looking movement

inlinable void deinit_navigator(const Navigator nav) {
	if (nav.path_ind != -1) wfree(nav.path_to_player.data);
}

inlinable void update_path(Navigator* const nav, const VectorF player_pos, const byte first_update) {
	const ResultBFS bfs_result = bfs(*nav -> pos, player_pos);

	if (bfs_result.succeeded) {
		if (!first_update) deinit_navigator(*nav);
		const Path path = bfs_result.path;
		nav -> path_to_player = path;
		nav -> path_ind = 0;
	}
}

inlinable Navigator init_navigator(const VectorF player_pos, VectorF* const pos_ref,
	double* const dist_to_player_ref, const double v) {

	Navigator nav = {.pos = pos_ref, .dist_to_player = dist_to_player_ref, .path_ind = -1, .v = v};
	update_path(&nav, player_pos, 1);
	return nav;
}

inlinable byte navigator_in_wall(const VectorF pos) { // 8 directions
	return map_point(current_level.wall_data, pos[0] + 0.5, pos[1])
		|| map_point(current_level.wall_data, pos[0] - 0.5, pos[1])
		|| map_point(current_level.wall_data, pos[0], pos[1] + 0.5)
		|| map_point(current_level.wall_data, pos[0], pos[1] - 0.5)
		|| map_point(current_level.wall_data, pos[0] + 0.5, pos[1] + 0.5)
		|| map_point(current_level.wall_data, pos[0] + 0.5, pos[1] - 0.5)
		|| map_point(current_level.wall_data, pos[0] - 0.5, pos[1] + 0.5)
		|| map_point(current_level.wall_data, pos[0] - 0.5, pos[1] - 0.5);
}

/*
void construct_non_clipping_path(Path path) {
	const VectorI* orig_path = path.data;
	VectorF* const better_path = malloc(path.length * sizeof(VectorF));
	for (int i = 0; i < path.length; i++) {
		const VectorI orig_path_entry = orig_path[i];
		better_path[i] = (VectorF) {orig_path_entry.x + 0.5, orig_path_entry.y + 0.5};
	}
}
*/

// construct a new path that has been guaranteed to not have any walls to clip into on its way
NavigatorState update_path_if_needed(Navigator* const nav, const VectorF player_pos, const Jump jump) {
	if (jump.height > 0.0 && !jump.jumping) return CouldNotNavigate;
	else if (nav -> path_ind == -1) {
		update_path(nav, player_pos, 0);
		return CouldNotNavigate;
	}

	const Path* const path = &nav -> path_to_player;
	if (*nav -> dist_to_player >= 0.99) update_path(nav, player_pos, 0);

	if (nav -> path_ind < path -> length - 1) {
		const VectorI
			curr_vertexI = path -> data[nav -> path_ind],
			next_vertexI = path -> data[nav -> path_ind + 1];

		const VectorI dir = {next_vertexI.x - curr_vertexI.x, next_vertexI.y - curr_vertexI.y};

		VectorF* const pos_ref = nav -> pos;
		VectorF pos = *pos_ref;
		const VectorF movement = {nav -> v * dir.x, nav -> v * dir.y};

		pos = VectorFF_add(pos, movement);

		// if past the current vertex
		if ((dir.x == 1 && pos[0] >= next_vertexI.x) || (dir.x == -1 && pos[0] <= next_vertexI.x)
			|| (dir.y == 1 && pos[1] >= next_vertexI.y) || (dir.y == -1 && pos[1] <= next_vertexI.y)) nav -> path_ind++;

		*pos_ref = pos;
		return Navigating;
	}
	return ReachedDest;
}
