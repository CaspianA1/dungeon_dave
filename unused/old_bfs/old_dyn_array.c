DynArray (*init_dyn_array) (const int, ...) = init_queue;
void (*deinit_dyn_array) (Queue) = deinit_queue;

inlinable void push_dyn_array(DynArray* dyn_array, void* elem) {
	if (++dyn_array -> length == dyn_array -> max_alloc)
		dyn_array -> data = realloc(dyn_array -> data, ++dyn_array -> max_alloc * sizeof(void*));

	dyn_array -> data[dyn_array -> length - 1] = elem;
}

inlinable DynArray copy_dyn_array(const DynArray src) {
	// printf("\tCopy allocation of dyn array\n");
	DynArray copy = {malloc(src.length * sizeof(void*)), src.length, src.length};
	// memcpy(copy.data, src.data, src.length * sizeof(void*));
	for (int i = 0; i < src.length; i++) copy.data[i] = src.data[i];
	return copy;
}
