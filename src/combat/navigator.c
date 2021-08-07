/*
static void print_navigator(const CorrectedRoute corrected_route, const vec floating_pos) {
	const ivec
		pos = ivec_from_vec(floating_pos),
		end = ivec_from_vec(corrected_route.data[corrected_route.length - 1]);

	for (int y = 0; y < current_level.map_size.y; y++) {
		for (int x = 0; x < current_level.map_size.x; x++) {
			const byte wall = map_point(current_level.wall_data, x, y);
			byte color;

			if (pos.x == x && pos.y == y) color = 200;
			else if (wall) color = 0;
			else {
				byte visited_point = 0;
				for (int i = 0; i < corrected_route.length; i++) {
					const ivec node = ivec_from_vec(corrected_route.data[i]);
					if (node.x == x && node.y == y) {
						visited_point = 1;
						break;
					}
				}

				if (visited_point) color = 118;
				else if (x == end.x && y == end.y) color = 208;
				else color = 99;
			}
			printf("\033[48;5;%dm \033[0m", color);
		}
		putchar('\n');
	}
	printf("---\n");
}
*/

static CorrectedRoute make_corrected_route(const Route route, const vec nav_pos) {
	CorrectedRoute corrected_route = {.length = route.length};

	corrected_route.data[0] = nav_pos;
	for (int i = 1; i < route.length; i++) {
		const ivec orig_node = route.data[i];
		corrected_route.data[i] = (vec) {orig_node.x, orig_node.y} + vec_fill(0.5);
	}

	return corrected_route;
}

// navigator idle if path too long
inlinable NavigationState update_route(Navigator* const nav, const vec player_pos) {
	const vec nav_pos = *nav -> pos;
	const ResultBFS bfs_result = bfs(nav_pos, player_pos); // allocate integer BFS path

	if (bfs_result.state == SucceededBFS) {
		const CorrectedRoute corrected_route = make_corrected_route(bfs_result.route, nav_pos);
		nav -> route = corrected_route;
		nav -> route_ind = 0;
	}
	return bfs_result.state;
}

inlinable Navigator init_navigator(const vec player_pos, vec* const pos_ref, const double v) {
	Navigator nav = {.pos = pos_ref, .route_ind = -1, .v = v};
	update_route(&nav, player_pos);
	return nav;
}

NavigationState update_path_if_needed(Navigator* const nav, const vec player_pos) {
	const byte base_height = current_level.get_point_height(
		map_point(current_level.wall_data, player_pos[0], player_pos[1]), player_pos);

	if (base_height > 0.0 || (nav -> route_ind == -1 && update_route(nav, player_pos) != SucceededBFS))
		return FailedBFS;

	const CorrectedRoute* const route = &nav -> route;

	const int end_ind = route -> length - 1;
	if (vec_delta_exceeds(player_pos, route -> data[end_ind], 1.0)) {
		const NavigationState nav_state = update_route(nav, player_pos);
		if (nav_state != SucceededBFS) return nav_state;
	}

	if (nav -> route_ind < end_ind) {
		const vec
			curr_vertex = route -> data[nav -> route_ind],
			next_vertex = route -> data[nav -> route_ind + 1];

		const vec dir = next_vertex - curr_vertex;
		vec* const ref_pos = nav -> pos;
		vec pos = *ref_pos + dir * vec_fill(nav -> v);

		if ((dir[0] > 0.0 && pos[0] >= next_vertex[0]) || (dir[0] < 0.0 && pos[0] <= next_vertex[0])
			|| (dir[1] > 0.0 && pos[1] >= next_vertex[1]) || (dir[1] < 0.0 && pos[1] <= next_vertex[1]))
			nav -> route_ind++;

		*ref_pos = pos;
		return Navigating;
	}
	return ReachedDest;
}
