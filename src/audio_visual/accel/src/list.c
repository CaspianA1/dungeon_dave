#ifndef LIST_C
#define LIST_C

#include <stdlib.h>

typedef struct {
	void* data;
	size_t datum_size, length, max_alloc;
} List;

#endif
