#ifndef ALLOC_H
#define ALLOC_H

// TODO: perhaps also track OpenGL allocations

#include <stdlib.h> // For `size_t`, and allocation functions
#include "data/constants.h" // For `TRACK_MEMORY`

#ifdef TRACK_MEMORY
	// Excluded: memory_report
	void* alloc(const size_t num_items, const size_t size);
	void* clearing_alloc(const size_t num_items, const size_t size);
	void* resizing_alloc(void* const memory, const size_t num_bytes);
	void dealloc(void* const memory);
#else
	static inline void* alloc(const size_t num_items, const size_t size) {
		return malloc(num_items * size);
	}

	#define clearing_alloc calloc
	#define resizing_alloc realloc
	#define dealloc free
#endif
#endif
