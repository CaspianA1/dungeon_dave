// need actual wall collision detection for viable good-looking movement

inlinable void deinit_navigator(const Navigator nav) {
	if (nav.path_ind != -1) wfree(nav.path_to_player.data);
}

inlinable void update_path(Navigator* nav, const VectorF player_pos, const byte first_update) {
	const ResultBFS bfs_result = bfs(*nav -> pos, player_pos);

	if (bfs_result.succeeded) {
		if (!first_update) deinit_navigator(*nav);
		const Path path = bfs_result.path;
		nav -> path_to_player = path;
		nav -> path_ind = 0;
	}
}

inlinable Navigator init_navigator(const VectorF player_pos, VectorF* pos_ref, const double v) {
	Navigator nav = {.pos = pos_ref, .path_ind = -1, .v = v};
	update_path(&nav, player_pos, 1);
	return nav;
}

// returns if the destination was reached
NavigatorState update_path_if_needed(Navigator* nav, const VectorF player_pos, const Jump jump) {
	if (jump.height > 0.0 && !jump.jumping) return CouldNotNavigate;
	else if (nav -> path_ind == -1) {
		update_path(nav, player_pos, 0);
		return CouldNotNavigate;
	}

	const Path* path = &nav -> path_to_player;
	const VectorI dest_vertex = path -> data[path -> length - 1];

	const VectorF player_dest_delta = {
		fabs(player_pos[0] - dest_vertex.x), fabs(player_pos[1] - dest_vertex.y)
	};

	if (player_dest_delta[0] >= 0.99 || player_dest_delta[1] >= 0.99)
		update_path(nav, player_pos, 0);

	if (nav -> path_ind < path -> length - 1) {
		const VectorI
			curr_vertexI = path -> data[nav -> path_ind],
			next_vertexI = path -> data[nav -> path_ind + 1];

		const VectorI dir = {next_vertexI.x - curr_vertexI.x, next_vertexI.y - curr_vertexI.y};
		const VectorF next_vertex = {next_vertexI.x + 0.5, next_vertexI.y + 0.5};

		VectorF* pos_ref = nav -> pos;
		VectorF pos = *pos_ref;

		pos[0] += nav -> v * dir.x;
		pos[1] += nav -> v * dir.y;

		// if past the current vertex
		if ((dir.x == 1 && pos[0] >= next_vertex[0]) ||
			(dir.x == -1 && pos[0] <= next_vertex[0]) ||
			(dir.y == 1 && pos[1] >= next_vertex[1]) ||
			(dir.y == -1 && pos[1] <= next_vertex[1])) {

			pos[0] = floor(pos[0]);
			pos[1] = floor(pos[1]);
			pos = VectorFF_add(pos, VectorF_memset(0.5));

			nav -> path_ind++;
		}
		*pos_ref = pos;
		return Navigating;
	}
	return ReachedDest;
}
