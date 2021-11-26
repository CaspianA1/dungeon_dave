#ifndef LIST_C
#define LIST_C

const float list_realloc_rate = 1.5f;

typedef struct {
	void* data;
	size_t item_size, length, max_alloc;
} List;

#define deinit_list(list) free(list.data)

List _init_list(const size_t init_alloc, const size_t item_size) {
	if (init_alloc == 0) {
		fprintf(stderr, "Initial list size must be above zero\n");
		exit(0);
	}

	return (List) {
		malloc(init_alloc * item_size),
		item_size,
		0, init_alloc
	};
}

#define init_list(init_alloc, type) _init_list(init_alloc, sizeof(type))

void _push_ptr_to_list(List* const list, const void* const item_ptr) {
	const size_t item_size = list -> item_size;

	if (list -> length == list -> max_alloc) {
		const size_t new_max_alloc = ceilf(list -> max_alloc * list_realloc_rate);
		list -> max_alloc = new_max_alloc;
		list -> data = realloc(list -> data, new_max_alloc * item_size);
	}

	// Cast to byte, because offset is calculated bytewise
	void* const dest_begin = (byte*) list -> data + (list -> length++ * item_size);
	memcpy(dest_begin, item_ptr, item_size);
}

#define push_ptr_to_list(list, item) _push_ptr_to_list((list), (void*) (item))

#endif
