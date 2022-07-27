#include "alloc.h"

#ifdef TRACK_MEMORY

typedef uint32_t alloc_count_t;

static alloc_count_t
	num_allocs = 0, num_clearing_allocs = 0,
	num_resizing_allocs = 0, num_deallocs = 0;

static bool registered_memory_report = false;

static void memory_report(void) {
	const alloc_count_t total_allocs = num_allocs + num_clearing_allocs;

	alloc_count_t memory_status_number;
	const char* memory_status;

	//////////

	if (total_allocs > num_deallocs) {
		memory_status_number = total_allocs - num_deallocs;
		memory_status = "leak";
	}
	else if (num_deallocs > total_allocs) {
		memory_status_number = num_deallocs - total_allocs;
		memory_status = "double dealloc";
	}
	else {
		memory_status_number = 0;
		memory_status = "memory problem";
	}

	//////////

	printf("Memory report:\n"
		"There were %u total allocs, %u allocs, and %u clearing allocs.\n"
		"There were also %u resizing allocs and %u deallocs.\nFinal %s count: %u.\n---\n" ,
		total_allocs, num_allocs, num_clearing_allocs, num_resizing_allocs, num_deallocs,
		memory_status, memory_status_number
	);
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

void* resizing_alloc(void* const memory, const size_t num_bytes) {
	num_resizing_allocs++;
	return realloc(memory, num_bytes);
}

void dealloc(void* const memory) {
	num_deallocs++;
	free(memory);
}

#endif
