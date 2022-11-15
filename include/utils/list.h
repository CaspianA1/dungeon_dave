#ifndef LIST_H
#define LIST_H

#include "buffer_defs.h"
#include "alloc.h"

/* Users of List should not access `max_alloc`; it is
irrelevant to the user, and should be considered private. */
typedef struct {
	void* data;
	buffer_size_t item_size, length, max_alloc;
} List;

// Excluded: copy_to_list_end

// Note: calling `init_list` with `init_alloc` equal to 0 results in undefined behavior.
#define init_list(init_alloc, type) _init_list((init_alloc), sizeof(type))
#define deinit_list(list) dealloc((list).data)
#define value_at_list_index(list, index, type) ((type*) (list) -> data)[index]

#define LIST_FOR_EACH(initial_offset, list, item_name, out_of_bounds_name, ...) do {\
	const buffer_size_t length = (list) -> length, item_size = (list) -> item_size;\
	byte* const data = (byte*) (list) -> data;\
	byte* const out_of_bounds_name = data + length * item_size;\
	\
	for (byte* item_name = data + (initial_offset) * item_size; item_name < out_of_bounds_name; item_name += item_size)\
		do {__VA_ARGS__} while (false);\
} while (false)

List _init_list(const buffer_size_t init_alloc, const buffer_size_t item_size);
void push_ptr_to_list(List* const list, const void* const item_ptr);
void push_array_to_list(List* const list, const void* const items, const buffer_size_t num_items);

// TODO: remove if not used anywhere
static inline void* ptr_to_list_index(const List* const list, const buffer_size_t index) {
	return ((byte*) list -> data) + index * list -> item_size;
}

#endif
