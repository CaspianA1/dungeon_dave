#ifndef LIST_H
#define LIST_H

#include "buffer_defs.h"

typedef struct {
	void* data;
	buffer_size_t item_size, length, max_alloc;
} List;

// Excluded: copy_to_list_end

// Note: calling `init_list` with `init_alloc` equal to 0 results in undefined behavior.
#define init_list(init_alloc, type) _init_list((init_alloc), sizeof(type))
#define deinit_list(list) free((list).data)

List _init_list(const buffer_size_t init_alloc, const buffer_size_t item_size);
void push_ptr_to_list(List* const list, const void* const item_ptr);
void push_array_to_list(List* const list, const void* const items, const buffer_size_t num_items);
void* ptr_to_list_index(const List* const list, const buffer_size_t index);

#endif
