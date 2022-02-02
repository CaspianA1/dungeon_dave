#ifndef LIST_H
#define LIST_H

#include <stdio.h> // For reporting errors from push_ptr_to_list
#include "buffer_defs.h"

const GLfloat list_realloc_rate = 2.0f;

typedef struct {
	void* data;
	buffer_size_t item_size, length, max_alloc;
} List;

#define deinit_list(list) free(list.data)
#define init_list(init_alloc, type) _init_list(init_alloc, sizeof(type))
#define push_ptr_to_list(list, item) _push_ptr_to_list((list), (item))
#define value_at_list_index(list, index, type) *((type*) ptr_to_list_index((list), (index)))

List _init_list(const buffer_size_t init_alloc, const buffer_size_t item_size);
void _push_ptr_to_list(List* const list, const void* const item_ptr);
void* ptr_to_list_index(const List* const list, const buffer_size_t index);

//////////

#define LIST_INITIALIZER(subtype_name) init_##subtype_name##_list

#define LIST_INITIALIZER_SIGNATURE(subtype, subtype_name)\
	List LIST_INITIALIZER(subtype_name)(const buffer_size_t num_elems, ...)

#define DEF_LIST_INITIALIZER(subtype, subtype_name)\
	LIST_INITIALIZER_SIGNATURE(subtype, subtype_name) {\
		va_list args;\
		va_start(args, num_elems);\
		List list = init_list(num_elems, subtype);\
		for (buffer_size_t i = 0; i < num_elems; i++) {\
			const subtype elem = va_arg(args, subtype);\
			push_ptr_to_list(&list, &elem);\
		}\
		return list;\
	}

#endif
