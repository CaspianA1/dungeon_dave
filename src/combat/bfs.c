typedef enum {Top, Bottom, Left, Right, TopLeft, BottomLeft, TopRight, BottomRight} NeighborID;

inlinable byte neighbor_map_point(const ivec neighbors[8], const NeighborID neighbor_id) {
	const ivec neighbor = neighbors[neighbor_id];
	return map_point(current_level.wall_data, neighbor.x, neighbor.y);
}

void update_queue_with_neighbors(PathQueue* const paths, Path path, const ivec vertex, byte* const all_visited) {
	const ivec neighbors[8] = {
		{vertex.x, vertex.y - 1}, {vertex.x, vertex.y + 1},
		{vertex.x - 1, vertex.y}, {vertex.x + 1, vertex.y},
		{vertex.x - 1, vertex.y - 1}, {vertex.x - 1, vertex.y + 1},
		{vertex.x + 1, vertex.y - 1}, {vertex.x + 1, vertex.y + 1}
	};

	for (NeighborID i = 0; i < 8; i++) {
		byte skip_this_neighbor = 0;

		const ivec neighbor = neighbors[i];
		if (ivec_out_of_bounds(neighbor) || map_point(current_level.wall_data, neighbor.x, neighbor.y))
			skip_this_neighbor = 1;

		else if (i == BottomLeft) {
			if (neighbor_map_point(neighbors, Left) || neighbor_map_point(neighbors, Bottom))
				skip_this_neighbor = 1;
		}

		else if (i == BottomRight) {
			if (neighbor_map_point(neighbors, Bottom) || neighbor_map_point(neighbors, Right))
				skip_this_neighbor = 1;
		}

		else if (i == TopLeft) {
			if (neighbor_map_point(neighbors, Left) || neighbor_map_point(neighbors, Top))
				skip_this_neighbor = 1;
		}

		else if (i == TopRight) {
			if (neighbor_map_point(neighbors, Top) || neighbor_map_point(neighbors, Right))
				skip_this_neighbor = 1;
		}

		if (skip_this_neighbor) continue;

		byte* const was_visited = &all_visited[neighbor.y * current_level.map_size.x + neighbor.x];
		if (!*was_visited) {
			*was_visited = 1;
			Path path_copy = copy_path(path);
			add_to_path(&path_copy, neighbor);
			enqueue_a_path(paths, path_copy);
		}
	}
}

ResultBFS bfs(const vec begin, const vec end) {
	const ivec int_begin = ivec_from_vec(begin), int_end = ivec_from_vec(end);

	byte* const all_visited = wcalloc(current_level.map_size.x * current_level.map_size.y, sizeof(byte));
	set_map_point(all_visited, 1, int_begin.x, int_begin.y, current_level.map_size.x);

	PathQueue paths = init_path_queue(1, init_path(1, int_begin));
	ResultBFS result = {0};

	while (paths.length > 0) {
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

	return result;
}
