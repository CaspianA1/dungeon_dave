typedef enum {
	TopLeft, Top, TopRight,
	Left, Right,
	BottomLeft, Bottom, BottomRight
} NeighborID;

inlinable byte neighbor_map_point(const ivec neighbors[8], const NeighborID neighbor_id) {
	const ivec neighbor = neighbors[neighbor_id];
	return map_point(current_level.wall_data, neighbor.x, neighbor.y);
}

void update_queue_with_neighbors(PathQueue* const paths, Path path, const ivec vertex, byte* const all_visited) {
	const int dec_x = vertex.x - 1, dec_y = vertex.y - 1, inc_x = vertex.x + 1, inc_y = vertex.y + 1;
	const ivec neighbors[8] = {
		{dec_x, dec_y}, {vertex.x, dec_y}, {inc_x, dec_y},
		{dec_x, vertex.y}, {inc_x, vertex.y},
		{dec_x, inc_y}, {vertex.x, inc_y}, {inc_x, inc_y}
	};

	for (NeighborID i = 0; i < 8; i++) {
		const ivec neighbor = neighbors[i];

		if  ((ivec_out_of_bounds(neighbor) || map_point(current_level.wall_data, neighbor.x, neighbor.y)) ||
			(i == BottomLeft && (neighbor_map_point(neighbors, Left) || neighbor_map_point(neighbors, Bottom))) ||
			(i == BottomRight && (neighbor_map_point(neighbors, Bottom) || neighbor_map_point(neighbors, Right))) ||
			(i == TopLeft && (neighbor_map_point(neighbors, Left) || neighbor_map_point(neighbors, Top))) ||
			(i == TopRight && (neighbor_map_point(neighbors, Top) || neighbor_map_point(neighbors, Right))))
		continue;

		byte* const was_visited = &all_visited[neighbor.y * current_level.map_size.x + neighbor.x];
		if (!*was_visited) {
			*was_visited = 1;
			Path path_copy = copy_path(path);
			add_to_path(&path_copy, neighbor);
			enqueue_to_paths(paths, path_copy);
		}
	}
}

ResultBFS bfs(const vec begin, const vec end) {
	const ivec int_begin = ivec_from_vec(begin), int_end = ivec_from_vec(end);

	byte* const all_visited = wcalloc(current_level.map_size.x * current_level.map_size.y, sizeof(byte));
	set_map_point(all_visited, 1, int_begin.x, int_begin.y, current_level.map_size.x);

	PathQueue paths = init_path_queue(1, init_path(1, int_begin));
	ResultBFS result = {0};

	// unsigned bfs_allocs_for_iter = bfs_allocs;

	while (paths.length > 0) {
		// DEBUG(paths.length, d);

		Path path = dequeue_a_path(&paths);

		const ivec vertex = path.data[path.length - 1];

		if (vertex.x == int_end.x && vertex.y == int_end.y) {
			result.succeeded = 1;
			result.path = path;
			break;
		}

		update_queue_with_neighbors(&paths, path, vertex, all_visited);
		wfree(path.data);
	}

	wfree(all_visited);
	for (int i = 0; i < paths.length; i++) wfree(paths.data[i].data);
	wfree(paths.data);

	/*
	const unsigned curr_bfs_allocs = bfs_allocs - bfs_allocs_for_iter;
	DEBUG(curr_bfs_allocs, u);
	*/

	return result;
}
