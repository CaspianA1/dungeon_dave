#ifndef SAFE_IO_H
#define SAFE_IO_H

#include <stdio.h> // For `fopen`
#include "utils/failure.h" // For `FAIL`
#include "utils/alloc.h" // For `alloc`

static inline FILE* open_file_safely(const char* const path, const char* const mode) {
	FILE* const file = fopen(path, mode);
	if (file == NULL) FAIL(OpenFile, "could not open a file with the path of '%s'", path);
	return file;
}

static inline char* read_file_contents(const char* const path) {
	FILE* const file = open_file_safely(path, "r");

	/* TODO (possible bug): if `ftell` fails, `num_bytes` will
	underflow, and too much data will be allocated. */

	fseek(file, 0l, SEEK_END); // Set file position to end
	const size_t num_bytes = (size_t) ftell(file);
	fseek(file, 0l, SEEK_SET); // Rewind file position

	char* const data = alloc(num_bytes + 1l, sizeof(GLchar));
	fread(data, num_bytes, 1, file); // Read file bytes
	data[num_bytes] = '\0';

	fclose(file);

	return data;
}

#endif
