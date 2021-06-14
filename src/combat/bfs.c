byte vertex_is_valid(const VectorI vertex) {
	return
		vertex.x >= 0 && vertex.x < current_level.map_width
		&& vertex.y >= 0 && vertex.y < current_level.map_height
		&& !wall_point(vertex.x, vertex.y);
}

void update_queue_with_neighbors(
	PathQueue* paths, Path path, const VectorI vertex, byte** all_visited) {

	const VectorI
		top = {vertex.x, vertex.y - 1},
		bottom = {vertex.x, vertex.y + 1},
		left = {vertex.x - 1, vertex.y},
		right = {vertex.x + 1, vertex.y},

		top_left = {vertex.x - 1, vertex.y - 1},
		bottom_left = {vertex.x - 1, vertex.y + 1},
		top_right = {vertex.x + 1, vertex.y - 1},
		bottom_right = {vertex.x + 1, vertex.y + 1};

	const VectorI neighbors[8] = {
		top, bottom, left, right, top_left, bottom_left, top_right, bottom_right
	};

	// for no wall collisions, rule out movement with a corner wall and diagonal movement
	for (byte i = 0; i < 8; i++) {
		const VectorI neighbor = neighbors[i];
		if (vertex_is_valid(neighbor)) {
			byte** walls = current_level.wall_data;

			if ((VectorII_eq(neighbor, top_left) && walls[left.y][left.x]) ||
				(VectorII_eq(neighbor, top_right) && walls[right.y][right.x]) ||
				(VectorII_eq(neighbor, bottom_right) && walls[bottom.y][bottom.x]) ||
				(VectorII_eq(neighbor, bottom_left) && walls[bottom.y][bottom.x]))
				continue;

			byte* was_visited = &all_visited[neighbor.y][neighbor.x];
			if (!(*was_visited)) {
				*was_visited = 1;
				Path path_copy = copy_path(path);
				add_to_path(&path_copy, neighbor);
				enqueue_a_path(paths, path_copy);
			}
		}
	}
}

ResultBFS bfs(const VectorF begin, const VectorF end) {
	const VectorI
		int_begin = VectorF_floor(begin),
		int_end = VectorF_floor(end);

	/////
	byte** all_visited = wmalloc(current_level.map_height * sizeof(byte*));
	for (int y = 0; y < current_level.map_height; y++)
		all_visited[y] = wcalloc(current_level.map_width, sizeof(byte));
	all_visited[int_begin.y][int_begin.x] = 1;
	/////

	PathQueue paths = init_path_queue(1, init_path(1, int_begin));
	ResultBFS result = {.succeeded = 0};

	while (paths.length > 0) {
		Path path = dequeue_a_path(&paths);

		const VectorI vertex = path.data[path.length - 1];

		if (VectorII_eq(vertex, int_end)) {
			result.succeeded = 1;
			result.path = path;
			break;
		}

		update_queue_with_neighbors(&paths, path, vertex, all_visited);
		wfree(path.data);
	}

	for (int y = 0; y < current_level.map_height; y++) wfree(all_visited[y]);
	wfree(all_visited);

	for (int i = 0; i < paths.length; i++) wfree(paths.data[i].data);
	wfree(paths.data);

	return result;
}
