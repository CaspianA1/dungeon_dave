/*
static void print_navigator(const CorrectedRoute* const route, const vec floating_pos) {
	const ivec
		pos = ivec_from_vec(floating_pos),
		end = ivec_from_vec(route -> data[route -> length - 1]);

	for (int y = 0; y < current_level.map_size.y; y++) {
		for (int x = 0; x < current_level.map_size.x; x++) {
			const byte wall = *map_point(current_level.wallmap, x, y);
			byte color;

			if (pos.x == x && pos.y == y) color = 200;
			else if (wall) color = 0;
			else {
				byte visited_point = 0;
				for (int i = 0; i < route -> length; i++) {
					const ivec node = ivec_from_vec(route -> data[i]);
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

inlinable CorrectedRoute make_corrected_route(const Route* const route, const vec nav_pos) {
	CorrectedRoute corrected_route = {.length = route -> length};

	corrected_route.data[0] = nav_pos;
	for (int i = 1; i < corrected_route.length; i++)
		corrected_route.data[i] = vec_from_ivec(route -> data[i]) + vec_fill(0.5);

	return corrected_route;
}

inlinable NavigationState update_route(Navigator* const nav, const vec p_pos, const byte height) {
	const vec nav_pos = *nav -> pos;
	const ResultBFS bfs_result = bfs(nav_pos, p_pos, height); // allocate integer BFS path

	if (bfs_result.state == SucceededBFS) {
		const CorrectedRoute corrected_route = make_corrected_route(&bfs_result.route, nav_pos);
		nav -> route = corrected_route;
		nav -> route_ind = 0;
	}

	return bfs_result.state;
}

inlinable Navigator init_navigator(const vec p_pos, vec* const pos_ref, const double v, const byte height) {
	Navigator nav = {.route_ind = -1, .pos = pos_ref, .v = v};
	update_route(&nav, p_pos, height);
	return nav;
}

inlinable byte player_diverged_from_route_dest(const CorrectedRoute* const route, const vec p_pos) {
	return vec_delta_exceeds(p_pos, route -> data[route -> length - 1], enemy_dist_for_attack);
}

NavigationState update_route_if_needed(Navigator* const nav, const vec p_pos, const byte height) {
	if (nav -> route_ind == -1 && update_route(nav, p_pos, height) != SucceededBFS)
		return FailedBFS;

	const CorrectedRoute* const route = &nav -> route;

	if (player_diverged_from_route_dest(route, p_pos)) {
		const NavigationState nav_state = update_route(nav, p_pos, height);
		if (nav_state != SucceededBFS) return nav_state;
	}

	const int route_ind = nav -> route_ind;
	if (route_ind < route -> length - 1) {
		const vec next_vertex = route -> data[route_ind + 1];
		const vec dir = next_vertex - route -> data[route_ind];

		vec* const pos_ref = nav -> pos;
		vec new_pos = *pos_ref + dir * vec_fill(nav -> v);

		// If the navigator has passed the next vertex
		if ((dir[0] > 0.0 && new_pos[0] >= next_vertex[0]) || (dir[0] < 0.0 && new_pos[0] <= next_vertex[0])
			|| (dir[1] > 0.0 && new_pos[1] >= next_vertex[1]) || (dir[1] < 0.0 && new_pos[1] <= next_vertex[1])) {

			/* Fully aligns the new position to the middle
			of the tile, to guarantee no wall clipping */
			new_pos = next_vertex;
			nav -> route_ind++;
		}

		*pos_ref = new_pos;
		return Navigating;
	}
	return ReachedDest;
}