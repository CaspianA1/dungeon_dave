Route extend_route(const Route* const route, const ivec node) {
	Route route_copy = *route;

	if (++route_copy.length == max_route_length)
		route_copy.creation_error = 1;
	else
		route_copy.data[route_copy.length - 1] = node;

	return route_copy;
}

RouteQueue init_routes(const ivec starting_node) {
	RouteQueue routes = {wmalloc(route_queue_init_alloc * sizeof(Route)), 1, route_queue_init_alloc};
	routes.data[0] = (Route) {0, 1, {starting_node}};
	return routes;
}

#define deinit_routes(routes) wfree(routes.data)

void enqueue_to_routes(RouteQueue* const routes_ref, const Route route) {
	RouteQueue routes = *routes_ref;

	if (routes.length++ == routes.max_alloc) {
		routes.max_alloc *= 2;
		routes.data = wrealloc(routes.data, routes.max_alloc * sizeof(Route));
	}

	for (int i = routes.length - 1; i > 0; i--)
		routes.data[i] = routes.data[i - 1];
	routes.data[0] = route;

	*routes_ref = routes;
}

Route* dequeue_from_routes(RouteQueue* const routes_ref) {
	if (routes_ref -> length == 0) FAIL("Cannot dequeue from an empty route queue\n");
	return &routes_ref -> data[--routes_ref -> length];
}
