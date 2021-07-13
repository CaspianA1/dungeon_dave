void print_navigator(const CorrectedPath corrected_path, const vec floating_pos) {
	const ivec
		pos = ivec_from_vec(floating_pos),
		end = ivec_from_vec(corrected_path.data[corrected_path.length - 1]);

	for (int y = 0; y < current_level.map_size.y; y++) {
		for (int x = 0; x < current_level.map_size.x; x++) {
			const byte wall = map_point(current_level.wall_data, x, y);
			byte color;

			if (pos.x == x && pos.y == y) color = 200;
			else if (wall) color = 0;
			else {
				byte visited_point = 0;
				for (int i = 0; i < corrected_path.length; i++) {
					const ivec node = ivec_from_vec(corrected_path.data[i]);
					if (node.x == x && node.y == y) {
						visited_point = 1;
						break;
					}
				}

				if (visited_point) color = 118;
				else if (x == end.x == y == end.y) color = 208;
				else color = 99;
			}
			printf("\033[48;5;%dm \033[0m", color);
		}
		putchar('\n');
	}
	printf("---\n");
}

CorrectedPath make_corrected_path(const Path path, const vec nav_pos) {
	const CorrectedPath corrected_path = {wmalloc(path.length * sizeof(vec)), path.length};

	corrected_path.data[0] = nav_pos;
	for (int i = 1; i < path.length; i++) {
		const ivec orig_entry = path.data[i];
		corrected_path.data[i] = (vec) {orig_entry.x, orig_entry.y} + vec_fill(0.5);
	}

	return corrected_path;
}

inlinable void deinit_navigator(const Navigator* nav) {
	if (nav -> path_ind != -1) wfree(nav -> path.data);
}

inlinable byte update_path(Navigator* const nav, const vec player_pos, const byte first_update) {
	const vec nav_pos = *nav -> pos;
	const ResultBFS bfs_result = bfs(nav_pos, player_pos); // allocate integer BFS path

	if (bfs_result.succeeded) {
		// allocate float BFS path
		const CorrectedPath corrected_path = make_corrected_path(bfs_result.path, nav_pos);
		wfree(bfs_result.path.data); // free integer BFS path

		if (!first_update) deinit_navigator(nav); // free float BFS path
		nav -> path = corrected_path;
		nav -> path_ind = 0;
	}
	return bfs_result.succeeded;
}

inlinable Navigator init_navigator(const vec player_pos, vec* const pos_ref, const double v) {
	Navigator nav = {.pos = pos_ref, .path_ind = -1, .v = v};
	update_path(&nav, player_pos, 1);
	return nav;
}

NavigatorState update_path_if_needed(Navigator* const nav, const vec player_pos, const double p_height) {
	if (p_height > 0.0 || (nav -> path_ind == -1 && !update_path(nav, player_pos, 0)))
		return CouldNotNavigate;

	const CorrectedPath* const path = &nav -> path;

	const int end_ind = path -> length - 1;
	if (vec_delta_exceeds(player_pos, path -> data[end_ind], 1.0)) {
		if (!update_path(nav, player_pos, 0)) return CouldNotNavigate;
	}

	/*
	print_navigator(nav -> path, *nav -> pos);
	DEBUG_VEC((*nav -> pos));
	*/

	if (nav -> path_ind < end_ind) {
		const vec
			curr_vertex = path -> data[nav -> path_ind],
			next_vertex = path -> data[nav -> path_ind + 1];

		const vec dir = next_vertex - curr_vertex;
		vec* const ref_pos = nav -> pos;
		vec pos = *ref_pos + dir * vec_fill(nav -> v);

		if ((dir[0] > 0.0 && pos[0] >= next_vertex[0]) || (dir[0] < 0.0 && pos[0] <= next_vertex[0])
			|| (dir[1] > 0.0 && pos[1] >= next_vertex[1]) || (dir[1] < 0.0 && pos[1] <= next_vertex[1]))
			nav -> path_ind++;

		*ref_pos = pos;
		return Navigating;
	}
	return ReachedDest;
}
