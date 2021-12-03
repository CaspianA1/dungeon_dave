#ifndef LIST_H
#define LIST_H

#include "utils.h"

const float list_realloc_rate = 2.0f;

typedef struct {
	void* data;
	size_t item_size, length, max_alloc;
} List;

#define deinit_list(list) free(list.data)
#define init_list(init_alloc, type) _init_list(init_alloc, sizeof(type))
#define push_ptr_to_list(list, item) _push_ptr_to_list((list), (void*) (item))

List _init_list(const size_t init_alloc, const size_t item_size);
void _push_ptr_to_list(List* const list, const void* const item_ptr);

#endif
