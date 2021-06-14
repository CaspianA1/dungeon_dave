Path init_path(const int init_length, ...) {
	va_list args;
	va_start(args, init_length);

	Path path = {wmalloc(init_length * sizeof(VectorI)), init_length, init_length};
	for (int i = 0; i < init_length; i++) path.data[i] = va_arg(args, VectorI);

	va_end(args);
	return path;
}

inlinable Path copy_path(const Path path) {
	Path copy = {wmalloc(path.length * sizeof(VectorI)), path.length, path.length};
	memcpy(copy.data, path.data, path.length * sizeof(VectorI));
	return copy;
}

inlinable void add_to_path(Path* path, const VectorI new) {
	if (path -> length++ == path -> max_alloc)
		path -> data = wrealloc(path -> data, ++path -> max_alloc * sizeof(VectorI));

	path -> data[path -> length - 1] = new;
}

/////

PathQueue init_path_queue(const int init_length, ...) {
	va_list args;
	va_start(args, init_length);

	PathQueue path_queue = {wmalloc(init_length * sizeof(Path)), init_length, init_length};
	for (int i = 0; i < init_length; i++) path_queue.data[i] = va_arg(args, Path);

	va_end(args);
	return path_queue;
}

inlinable void enqueue_a_path(PathQueue* path_queue, const Path new) {
	if (path_queue -> length++ == path_queue -> max_alloc)
		path_queue -> data = wrealloc(path_queue -> data,
			++path_queue -> max_alloc * sizeof(Path));

	for (int i = path_queue -> length - 1; i > 0; i--)
		path_queue -> data[i] = path_queue -> data[i - 1];

	path_queue -> data[0] = new;
}

inlinable Path dequeue_a_path(PathQueue* path_queue) {
	if (path_queue -> length == 0) FAIL("Cannot dequeue from an empty queue!\n");
	return path_queue -> data[--path_queue -> length];
}
