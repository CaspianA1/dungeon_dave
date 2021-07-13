enum {max_route_length = 8};

typedef struct {
	int length;
	ivec data[max_route_length];
} Route;

/*
for the queue {a b c d e},
an enqueue of f becomes {f a b c d e},
and a dequeue becomes {f a b c d}
*/

