#ifndef SAFE_IO_H
#define SAFE_IO_H

#include <stdio.h> // For `fopen`
#include "utils/failure.h" // For `FAIL`

static inline FILE* open_file_safely(const char* const path, const char* const mode) {
	FILE* const file = fopen(path, mode);
	if (file == NULL) FAIL(OpenFile, "could not open a file with the path of '%s'", path);
	return file;
}


#endif
