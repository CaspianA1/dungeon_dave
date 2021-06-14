#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#define infinity 1000

struct Node {
	unsigned char value;
	int visited, distance, edge_count;
	int* weights, weight_assign_index, freed;
	struct Node** edges;
};

typedef struct Node Node;

Node* init_node(const unsigned char value, const int edge_count) {
	Node* node = malloc(sizeof(Node));

	node -> value = value;
	node -> visited = 0;
	node -> distance = infinity;
	node -> edge_count = edge_count;

	node -> weights = malloc(edge_count * sizeof(int));
	node -> weight_assign_index = 0;
	node -> freed = 0;

	node -> edges = malloc(edge_count * sizeof(Node*));

	return node;
}

void assign_edges(Node* node, const int amount, ...) {
	va_list edges;
	va_start(edges, amount);

	for (int i = 0; i < amount; i++)
		node -> edges[i] = va_arg(edges, Node*);

	va_end(edges);
}

void assign_weight(Node* node_1, Node* node_2, const int weight) {
	for (int i = 0; i < node_1 -> edge_count; i++) {
		if (node_1 -> edges[i] == node_2) {
			node_1 -> weights[node_1 -> weight_assign_index++] = weight;
			node_2 -> weights[node_2 -> weight_assign_index++] = weight;
		}
	}
}

void deinit_graph(Node* node) {
	if (!node -> freed) {
		node -> freed = 1;
		free(node -> weights);
		for (int i = 0; i < node -> edge_count; i++)
			deinit_graph(node -> edges[i]);
		free(node -> edges);
	}
}

void dijkstra(Node* current, Node* goal) {
	Node local_closest;
	local_closest.distance = infinity;
	Node* closest = &local_closest;

	printf("Current: %c\n", current -> value);

	for (int i = 0; i < current -> edge_count; i++) {
		Node* neighbor = current -> edges[i];
		if (!neighbor -> visited) {
			printf("New neighbor: %c\n", neighbor -> value);
			const int tentative_distance = current -> distance + current -> weights[i];

			if (tentative_distance < neighbor -> distance)
				neighbor -> distance = tentative_distance;

			if (neighbor -> distance < closest -> distance)
				closest = neighbor;
		}
	}
	printf("%c, closest, distance of %d\n", closest -> value, closest -> distance);

	current -> visited = 1;
	if (closest == goal) printf("Shortest distance is %d\n", closest -> distance);
	else dijkstra(closest, goal);
}

int main() {
	Node
		*a = init_node('a', 2),
		*b = init_node('b', 3),
		*c = init_node('c', 3),
		*d = init_node('d', 2),
		*e = init_node('e', 2),
		*f = init_node('f', 2),
		*g = init_node('g', 2),
		*h = init_node('h', 2);

	assign_edges(a, 2, e, b);
	assign_edges(b, 3, a, f, c);
	assign_edges(c, 3, b, h, d);
	assign_edges(d, 2, c, g);
	assign_edges(e, 2, a, g);
	assign_edges(f, 2, b, h);
	assign_edges(g, 2, e, d);
	assign_edges(h, 2, f, c);

	assign_weight(a, e, 2);
	assign_weight(a, b, 4);
	assign_weight(b, c, 1);
	assign_weight(b, f, 1);
	assign_weight(f, h, 1);
	assign_weight(h, c, 1);
	assign_weight(c, d, 2);
	assign_weight(d, g, 1);
	assign_weight(g, e, 1);


	e -> distance = 0;
	dijkstra(e, f);
	deinit_graph(a);
}

/*

       F - H
       |   |
A ---- B - C
|         /
|        /
E - G - D

*/

/*
crashes:
	(e, f)
	(f, d)
	(c, a)
	(f, g)
incorrect:
	(a, c)
	(g, f)
working:
	(d, f)
*/

// make dijkstra work, and then use bfs