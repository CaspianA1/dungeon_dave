#ifndef ALLOC_C
#define ALLOC_C

#include "headers/alloc.h"

#ifdef TRACK_MEMORY

static uint32_t
	num_allocs = 0, num_clearing_allocs = 0,
	num_resize_allocs = 0, num_deallocs = 0;

static bool registered_memory_report = false;

static void memory_report(void) {
	const uint32_t total_allocations = num_allocs + num_clearing_allocs;

	printf(
		"There were %u total allocations, %u allocs, and %u clearing allocs.\n"
		"There were also %u reallocations and %u deallocs.\n",
		total_allocations, num_allocs, num_clearing_allocs,
		num_resize_allocs, num_deallocs
	);

	if (total_allocations > num_deallocs)
		printf("%u memory blocks were leaked!\n", total_allocations - num_deallocs);

	else if (num_deallocs > total_allocations)
		printf("%u memory blocks were mistakenly freed!\n", num_deallocs - total_allocations);
}

void* alloc(const size_t num_items, const size_t size) {
	num_allocs++;

	if (!registered_memory_report) {
		atexit(memory_report);
		registered_memory_report = true;
	}

	return malloc(num_items * size);
}

void* clearing_alloc(const size_t num_items, const size_t size) {
	num_clearing_allocs++;

	if (!registered_memory_report) {
		atexit(memory_report);
		registered_memory_report = true;
	}

	return calloc(num_items, size);
}

void* resize_alloc(void* const memory, const size_t num_bytes) {
	num_resize_allocs++;
	return realloc(memory, num_bytes);
}

void dealloc(void* const memory) {
	num_deallocs++;
	free(memory);
}

#endif
#endif
