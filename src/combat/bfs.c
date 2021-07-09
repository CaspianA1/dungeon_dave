byte vertex_is_valid(const ivec vertex) {
	return
		vertex.x >= 0 && vertex.x < current_level.map_width
		&& vertex.y >= 0 && vertex.y < current_level.map_height
		&& !map_point(current_level.wall_data, vertex.x, vertex.y); // use the other fn
}

void update_queue_with_neighbors(PathQueue* const paths, Path path, const ivec vertex, byte* const all_visited) {
	/*
	const VectorI
		top = {vertex.x, vertex.y - 1},
		bottom = {vertex.x, vertex.y + 1},
		left = {vertex.x - 1, vertex.y},
		right = {vertex.x + 1, vertex.y},

		top_left = {vertex.x - 1, vertex.y - 1},
		bottom_left = {vertex.x - 1, vertex.y + 1},
		top_right = {vertex.x + 1, vertex.y - 1},
		bottom_right = {vertex.x + 1, vertex.y + 1};

	// use an enum
	const VectorI neighbors[8] = {
		top, bottom, left, right, top_left, bottom_left, top_right, bottom_right
	};
	*/

	const ivec neighbors[8] = {
		{vertex.x, vertex.y - 1}, {vertex.x, vertex.y + 1},
		{vertex.x - 1, vertex.y}, {vertex.x + 1, vertex.y},
		{vertex.x - 1, vertex.y - 1}, {vertex.x - 1, vertex.y + 1},
		{vertex.x + 1, vertex.y - 1}, {vertex.x + 1, vertex.y + 1}
	};

	typedef enum {Top, Bottom, Left, Right, TopLeft, BottomLeft, TopRight, BottomRight} NeighborID;

	// for no wall collisions, rule out movement with a corner wall and diagonal movement
	for (NeighborID i = 0; i < 8; i++) {
		const ivec neighbor = neighbors[i];
		if (vertex_is_valid(neighbor)) {
			if ((i == TopLeft) ||
				(i == TopRight) ||
				(i == BottomRight) ||
				(i == BottomLeft)) {}

			/*
			if ((VectorII_eq(neighbor, top_left) && map_point(current_level.wall_data, left.x, left.y)) ||
				(VectorII_eq(neighbor, top_right) && map_point(current_level.wall_data, right.x, right.y)) ||
				(VectorII_eq(neighbor, bottom_right) && map_point(current_level.wall_data, bottom.x, bottom.y)) ||
				(VectorII_eq(neighbor, bottom_left) && map_point(current_level.wall_data, bottom.x, bottom.y)))
				continue;
			*/

			byte* const was_visited = &all_visited[neighbor.y * current_level.map_width + neighbor.x];
			if (!*was_visited) {
				*was_visited = 1;
				Path path_copy = copy_path(path);
				add_to_path(&path_copy, neighbor);
				enqueue_a_path(paths, path_copy);
			}
		}
	}
}

ResultBFS bfs(const vec begin, const vec end) {
	const ivec int_begin = vec_to_ivec(begin), int_end = vec_to_ivec(end);

	byte* const all_visited = wcalloc(current_level.map_width * current_level.map_height, sizeof(byte));
	set_map_point(all_visited, 1, int_begin.x, int_begin.y, current_level.map_width);

	PathQueue paths = init_path_queue(1, init_path(1, int_begin));
	ResultBFS result = {.succeeded = 0};

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
