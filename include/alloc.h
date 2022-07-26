#ifndef ALLOC_H
#define ALLOC_H

// TODO: perhaps also track OpenGL allocations

#include "utils.h"

#ifdef TRACK_MEMORY

// Excluded: memory_report

void* alloc(const size_t num_items, const size_t size);
void* clearing_alloc(const size_t num_items, const size_t size);
void* resize_alloc(void* const memory, const size_t num_bytes);
void dealloc(void* const memory);

#else

static inline void* alloc(const size_t num_items, const size_t size) {
    return malloc(num_items * size);
}

#define clearing_alloc calloc
#define resize_alloc realloc
#define dealloc free

#endif
#endif
