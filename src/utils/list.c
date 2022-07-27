#include "utils/list.h"

#define LIST_REALLOC_AMOUNT_FOR(curr_alloc) ((buffer_size_t) (ceilf((curr_alloc) * list_realloc_rate)))

static const GLfloat list_realloc_rate = 2.0f;

//////////

List _init_list(const buffer_size_t init_alloc, const buffer_size_t item_size) {
	return (List) {alloc(init_alloc, item_size), item_size, 0, init_alloc};
}

static void copy_to_list_end(List* const list, const void* const data, const buffer_size_t num_items) {
	const buffer_size_t item_size = list -> item_size;

	// Cast to byte, because offset is calculated bytewise
	void* const dest_begin = (byte*) list -> data + list -> length * item_size;
	list -> length += num_items;
	memcpy(dest_begin, data, num_items * item_size);
}

void push_ptr_to_list(List* const list, const void* const item_ptr) {
	buffer_size_t max_alloc = list -> max_alloc;

	if (list -> length == max_alloc) {
		max_alloc = LIST_REALLOC_AMOUNT_FOR(max_alloc);
		list -> data = resizing_alloc(list -> data, max_alloc * list -> item_size);
		list -> max_alloc = max_alloc;
	}

	copy_to_list_end(list, item_ptr, 1);
}

void push_array_to_list(List* const list, const void* const items, const buffer_size_t num_items) {
	const buffer_size_t needed_alloc = list -> length + num_items;

	if (needed_alloc > list -> max_alloc) {
		const buffer_size_t needed_alloc_and_some_more = LIST_REALLOC_AMOUNT_FOR(needed_alloc);

		list -> max_alloc = needed_alloc_and_some_more;
		list -> data = resizing_alloc(list -> data, needed_alloc_and_some_more * list -> item_size);
	}

	copy_to_list_end(list, items, num_items);
}

void* ptr_to_list_index(const List* const list, const buffer_size_t index) {
	return ((byte*) list -> data) + index * list -> item_size;
}
