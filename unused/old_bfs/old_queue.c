typedef struct {
	void** data;
	int length, max_alloc;
} Queue;

typedef Queue DynArray;

Queue init_queue(const int init_length, ...) {
	// printf("\tAllocation of queue/vector\n");

	va_list elems;
	va_start(elems, init_length);

	Queue queue = {malloc(init_length * sizeof(void*)), init_length, init_length};
	for (int i = 0; i < init_length; i++)
		queue.data[i] = va_arg(elems, void*);

	va_end(elems);

	return queue;
}

inlinable void deinit_queue(Queue queue) {
	free(queue.data);
}

void print_paths(const Queue paths);

inlinable void enqueue(Queue* queue, void* elem) {
	printf("-----\n");
	printf("Before enqueueing: ");
	print_paths(*queue);

	if (++queue -> length == queue -> max_alloc)
		queue -> data = realloc(queue -> data, ++queue -> max_alloc * sizeof(void*));

	// see that this is actually shifting values over
	memcpy(queue -> data + 1, queue -> data, queue -> length * sizeof(void*));
	queue -> data[0] = elem;

	printf("After enqueueing: ");
	print_paths(*queue);
}

inlinable void* dequeue(Queue* queue) {
	if (queue -> length == 0) FAIL("Cannot dequeue from an empty queue\n");
	return queue -> data[--queue -> length];
}
