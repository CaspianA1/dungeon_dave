typedef enum {
	TopLeft, Top, TopRight,
	Left, Right,
	BottomLeft, Bottom, BottomRight
} NeighborID;

inlinable byte neighbor_map_point(const ivec neighbors[8], const NeighborID neighbor_id) {
	const ivec neighbor = neighbors[neighbor_id];
	return *map_point(current_level.wall_data, neighbor.x, neighbor.y);
}

// returns if updating the queue succeeded
inlinable byte update_queue_with_neighbors(RouteQueue* const routes, const Route* const route, const ivec vertex) {
	const int dec_x = vertex.x - 1, dec_y = vertex.y - 1, inc_x = vertex.x + 1, inc_y = vertex.y + 1;
	const ivec neighbors[8] = {
		{dec_x, dec_y}, {vertex.x, dec_y}, {inc_x, dec_y},
		{dec_x, vertex.y}, {inc_x, vertex.y},
		{dec_x, inc_y}, {vertex.x, inc_y}, {inc_x, inc_y}
	};

	for (NeighborID i = 0; i < 8; i++) {
		const ivec neighbor = neighbors[i];

		if  ((ivec_out_of_bounds(neighbor) || *map_point(current_level.wall_data, neighbor.x, neighbor.y)) ||
			(i == BottomLeft && (neighbor_map_point(neighbors, Left) || neighbor_map_point(neighbors, Bottom))) ||
			(i == BottomRight && (neighbor_map_point(neighbors, Bottom) || neighbor_map_point(neighbors, Right))) ||
			(i == TopLeft && (neighbor_map_point(neighbors, Left) || neighbor_map_point(neighbors, Top))) ||
			(i == TopRight && (neighbor_map_point(neighbors, Top) || neighbor_map_point(neighbors, Right))))
		continue;

		if (!set_statemap_bit_with_status(current_level.bfs_visited, neighbor.x, neighbor.y)) { // if not visited before
			const Route next_route = extend_route(route, neighbor);
			if (next_route.creation_error) return 0;
			enqueue_to_routes(routes, next_route);
		}
	}
	return 1;
}

ResultBFS bfs(const vec begin, const vec end) {
	const ivec int_begin = ivec_from_vec(begin), int_end = ivec_from_vec(end);

	clear_statemap(current_level.bfs_visited);
	set_statemap_bit(current_level.bfs_visited, int_begin.x, int_begin.y);

	RouteQueue routes = init_routes(int_begin);
	ResultBFS result = {.state = FailedBFS};

	while (routes.length > 0) {
		const Route route = *dequeue_from_routes(&routes);
		const ivec vertex = route.data[route.length - 1];

		if (vertex.x == int_end.x && vertex.y == int_end.y) {
			result.state = SucceededBFS;
			result.route = route;
			break;
		}

		if (!update_queue_with_neighbors(&routes, &route, vertex)) {
			result.state = PathTooLongBFS;
			break;
		}
	}

	deinit_routes(routes);
	return result;
}
