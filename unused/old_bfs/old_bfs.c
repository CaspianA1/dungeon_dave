#include "queue.c"
#include "dyn_array.c"

const int valid_vertex(const VectorI vertex) {
	return
		vertex.x >= 0 && vertex.x < current_level.map_width
		&& vertex.y >= 0 && vertex.y < current_level.map_height
		&& current_level.wall_data[vertex.y][vertex.x] == 0;
}

inlinable const int eq_VectorI(const VectorI v1, const VectorI v2) {
	return v1.x == v2.x && v1.y == v2.y;
}

inlinable void print_paths(Queue paths) {
	putchar('(');
	for (int i = 0; i < paths.length; i++) {
		putchar('[');
		const DynArray* path = paths.data[i];
		for (int j = 0; j < path -> length; j++) {
			const VectorI* vertex = path -> data[j];
			printf("{%d, %d}", vertex -> x, vertex -> y);
			if (j != path -> length - 1) printf(", ");
		}
		putchar(']');
	}
	printf(")\n");
}

inlinable void print_path(const DynArray path) {
	putchar('[');
	for (int i = 0; i < path.length; i++) {
		const VectorI* vertex = path.data[i];
		printf("{%d, %d}", vertex -> x, vertex -> y);
		if (i != path.length - 1) printf(", ");
	}
	printf("]\n");
}

inlinable int paths_eq(const DynArray* p1, const DynArray* p2) {
	if (p1 -> length != p2 -> length) return 0;

	for (int i = 0; i < p1 -> length; i++) {
		const VectorI *p1_v = p1 -> data[i], *p2_v = p2 -> data[i];
		if (p1_v -> x != p2_v -> x || p1_v -> y != p2_v -> y)
			return 0;
	}
	return 1;
}

void print_bfs(byte** visited, const VectorI* pos, const VectorI end) {
	for (int y = 0; y < current_level.map_height; y++) {
		for (int x = 0; x < current_level.map_width; x++) {
			const byte wall = current_level.wall_data[y][x];
			byte color;

			if (pos -> x == x && pos -> y == y) color = 200;
			else if (wall) color = 0;
			else if (visited[y][x]) color = 118;
			else if (x == end.x && y == end.y) color = 208;
			else color = 99;

			printf("\033[48;5;%dm \033[0m", color);

		}
		putchar('\n');
	}
	printf("---\n");
}

const DynArray* bfs(const VectorF begin, const VectorF end) {
	const VectorI
		int_begin = {(int) floor(begin.x), (int) floor(begin.y)},
		int_end = {(int) floor(end.x), (int) floor(end.y)};

	byte** visited = malloc(current_level.map_height * sizeof(byte*));
	for (int y = 0; y < current_level.map_height; y++)
		visited[y] = calloc(current_level.map_width, sizeof(byte));
	visited[int_begin.y][int_begin.x] = 1;

	DynArray base_path = init_dyn_array(1, (void*) &int_begin);
	Queue paths = init_queue(1, (void*) &base_path);

	// visualize path taken
	while (paths.length > 0) {
		printf("(top of code) Paths before dequeueing: ");
		print_paths(paths);

		DynArray* path = dequeue(&paths);
		printf("Paths after dequeueing: ");
		print_paths(paths);

		const VectorI* vertex = path -> data[path -> length - 1];
		printf("vertex = {%d, %d}\n", vertex -> x, vertex -> y);

		// if (!valid_vertex(*vertex)) continue;

		if (current_level.wall_data[vertex -> y][vertex -> x]) {
			printf("Error\n");
		}

		print_bfs(visited, vertex, int_end);

		if (eq_VectorI(*vertex, int_end))
			return path;

		const VectorI
			top = {vertex -> x, vertex -> y - 1},
			bottom = {vertex -> x, vertex -> y + 1},
			left = {vertex -> x - 1, vertex -> y},
			right = {vertex -> x + 1, vertex -> y};

			/*
			top_left = {vertex -> x - 1, vertex -> y - 1},
			bottom_left = {vertex -> x - 1, vertex -> y + 1},
			top_right = {vertex -> x + 1, vertex -> y - 1},
			bottom_right = {vertex -> x + 1, vertex -> y + 1};
			*/

		const VectorI possible_neighbors[4] = {
			top, bottom, left, right // top_left, bottom_left, top_right, bottom_right
		};

		for (int i = 0; i < 4; i++) {
			const VectorI possible_neighbor = possible_neighbors[i];
			if (valid_vertex(possible_neighbor)) {

				/*
				printf("maybe valid: {%d, %d}\n", possible_neighbor.x, possible_neighbor.y);
				if ((eq_VectorI(possible_neighbor, top_left) && current_level.wall_data[left.y][left.x]) ||
					(eq_VectorI(possible_neighbor, top_right) && current_level.wall_data[right.y][right.x]) ||
					(eq_VectorI(possible_neighbor, bottom_right) && current_level.wall_data[bottom.y][bottom.x]) ||
					(eq_VectorI(possible_neighbor, bottom_left) && current_level.wall_data[bottom.y][bottom.x])) {

					printf("skip candidate {%d, %d}\n", possible_neighbor.x, possible_neighbor.y);
					continue;
				}
				*/

				byte* visited_state = &visited[possible_neighbor.y][possible_neighbor.x];
				if (!(*visited_state)) {
					*visited_state = 1;

					printf("new_neighbor: {%d, %d}\n", possible_neighbor.x, possible_neighbor.y);

					DynArray path_copy = copy_dyn_array(*path);
					push_dyn_array(&path_copy, (void*) &possible_neighbor);
					printf("Path copy after pushing: "); print_path(path_copy);
					enqueue(&paths, &path_copy);
					printf("\n(bottom of code)\n\n");
				}
			}
		}
	}

	// for (int y = 0; y < current_level.map_height; y++) free(visited[y]);
	// free(visited);

	printf("Returning null\n");
	return NULL;
}

/*
---
Path copy after pushing: [{4, 11}, {4, 10}]
-----
Before enqueueing: paths = ()
After enqueueing: paths = ([{4, 11}, {4, 10}])
Path copy after pushing: [{4, 11}, {4, 12}]
-----
Before enqueueing: paths = ([{4, 11}, {4, 12}])
After enqueueing: paths = ([{4, 11}, {4, 12}][{4, 11}, {4, 12}])
Path copy after pushing: [{4, 11}, {3, 11}]
-----
Before enqueueing: paths = ([{4, 11}, {3, 11}][{4, 11}, {3, 11}])
After enqueueing: paths = ([{4, 11}, {3, 11}][{4, 11}, {3, 11}][{4, 11}, {3, 11}])
Path copy after pushing: [{4, 11}, {5, 11}]
-----
Before enqueueing: paths = ([{4, 11}, {5, 11}][{4, 11}, {5, 11}][{4, 11}, {5, 11}])
After enqueueing: paths = ([{4, 11}, {5, 11}][{4, 11}, {5, 11}][{4, 11}, {5, 11}][{4, 11}, {5, 11}])
Paths before dequeueing: paths = ([{4, 11}, {5, 12}][{4, 11}, {5, 12}][{4, 11}, {5, 12}][{4, 11}, {5, 12}])
Paths after dequeueing: paths = ([{4, 11}, {5, 12}][{4, 11}, {5, 12}][{4, 11}, {5, 12}])
vertex = {5, 12}
Error
*/
