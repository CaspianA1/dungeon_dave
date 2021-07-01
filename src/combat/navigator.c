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

CorrectedPath make_corrected_path(const Path path, const VectorF player_pos, const VectorF nav_pos) {
	const VectorI* const orig_path = path.data;
	const CorrectedPath corrected_path = {wmalloc(path.length * sizeof(VectorF)), path.length};
	const int end_ind = path.length - 1;

	corrected_path.data[0] = nav_pos;
	for (int i = 1; i < end_ind; i++) {
		const VectorI orig_entry = orig_path[i];
		corrected_path.data[i] = (VectorF) {orig_entry.x, orig_entry.y};
	}
	corrected_path.data[end_ind] = player_pos;

	return corrected_path;
}

inlinable void deinit_navigator(const Navigator* nav) {
	if (nav -> path_ind != -1) wfree(nav -> path.data);
}

inlinable byte update_path(Navigator* const nav, const VectorF player_pos, const byte first_update) {
	const VectorF nav_pos = *nav -> pos;
	const ResultBFS bfs_result = bfs(nav_pos, player_pos); // allocate integer BFS path

	if (bfs_result.succeeded) {
		// allocate float BFS path
		const CorrectedPath corrected_path = make_corrected_path(bfs_result.path, player_pos, nav_pos);
		wfree(bfs_result.path.data); // free integer BFS path

		if (!first_update) deinit_navigator(nav); // free float BFS path
		nav -> path = corrected_path;
		nav -> path_ind = 0;
	}
	return bfs_result.succeeded;
}

inlinable Navigator init_navigator(const VectorF player_pos, VectorF* const pos_ref,
	double* const dist_to_player_ref, const double v) {

	Navigator nav = {.pos = pos_ref, .dist_to_player = dist_to_player_ref, .path_ind = -1, .v = v};
	update_path(&nav, player_pos, 1);
	return nav;
}

/* construct a new path that has been guaranteed to not have any walls to clip into on its way;
for a given node on the path, if the enemy is in a wall during navigating, find a suitable replacement
node nearby - just keep trying to make that possible. but before that, don't allow the enemy BFS corner
movement, because that will remove many possible bugs later.
*/
NavigatorState update_path_if_needed(Navigator* const nav, const VectorF player_pos, const Jump jump) {
	if (jump.height > 0.0 || (nav -> path_ind == -1 && !update_path(nav, player_pos, 0)))
		return CouldNotNavigate;

	const CorrectedPath* const path = &nav -> path;

	const int end_ind = path -> length - 1;
	if (VectorFF_exceed_dist(player_pos, path -> data[end_ind], 1.0)) {
		if (!update_path(nav, player_pos, 0)) return CouldNotNavigate;
	}

	if (nav -> path_ind < end_ind) {
		const VectorF
			curr_vertex = path -> data[nav -> path_ind],
			next_vertex = path -> data[nav -> path_ind + 1];

		const VectorF dir = VectorFF_sub(next_vertex, curr_vertex);
		VectorF* const ref_pos = nav -> pos;
		const VectorF pos = VectorFF_add(*ref_pos, VectorFF_mul(dir, VectorF_memset(nav -> v)));

		if ((dir[0] > 0.0 && pos[0] >= next_vertex[0]) || (dir[0] < -0.0 && pos[0] <= next_vertex[0])
			|| (dir[1] > 0.0 && pos[1] >= next_vertex[1]) || (dir[1] < -0.0 && pos[1] <= next_vertex[1]))
			nav -> path_ind++;

		*ref_pos = pos;
		return Navigating;
	}
	return ReachedDest;
}
