#ifndef LIST_C
#define LIST_C

#include "headers/list.h"

static List _init_list(const buffer_size_t init_alloc, const buffer_size_t item_size) {
	return (List) {
		malloc(init_alloc * item_size),
		item_size,
		0, init_alloc
	};
}

static void copy_to_list_end(List* const list, const void* const data,
	const buffer_size_t num_bytes, const buffer_size_t num_items) {

	// Cast to byte, because offset is calculated bytewise
	void* const dest_begin = (byte*) list -> data + (list -> length * list -> item_size);
	list -> length += num_items;
	memcpy(dest_begin, data, num_bytes);
}

void push_ptr_to_list(List* const list, const void* const item_ptr) {
	const buffer_size_t item_size = list -> item_size;

	if (list -> length == list -> max_alloc) {
		const buffer_size_t new_max_alloc = LIST_REALLOC_AMOUNT_FOR(list -> max_alloc);

		list -> max_alloc = new_max_alloc;
		list -> data = realloc(list -> data, new_max_alloc * item_size);
	}

	copy_to_list_end(list, item_ptr, item_size, 1);
}

void push_array_to_list(List* const list, const void* const items, const buffer_size_t num_items) {
	const buffer_size_t datum_size = list -> item_size, needed_alloc = list -> length + num_items;

	if (needed_alloc > list -> max_alloc) {
		const buffer_size_t needed_alloc_and_some_more = LIST_REALLOC_AMOUNT_FOR(needed_alloc);

		list -> max_alloc = needed_alloc_and_some_more;
		list -> data = realloc(list -> data, needed_alloc_and_some_more * datum_size);
	}

	copy_to_list_end(list, items, num_items * datum_size, num_items);
}

void* ptr_to_list_index(const List* const list, const buffer_size_t index) {
	return ((byte*) list -> data) + index * list -> item_size;
}

#endif
