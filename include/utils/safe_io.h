#ifndef SAFE_IO_H
#define SAFE_IO_H

#include <stdio.h> // For `fopen`
#include "utils/failure.h" // For `FAIL`
#include "utils/alloc.h" // For `alloc`

static inline const char* get_temp_asset_path(const char* const unmodified_path) {
	// TODO: try to make these constants in `constants.h`
	static const char* const path_prefix = "../assets/";
	static const size_t max_concatenation_buffer_size = 100u;

	static char temp_concatenated_string[max_concatenation_buffer_size];

	const size_t total_concat_bytes = strlen(unmodified_path) + strlen(path_prefix) + 1u; // 1 more for the null terminator

	if (total_concat_bytes > max_concatenation_buffer_size)
		FAIL(OpenFile, "Cannot open the file with path '%s', since its path when prefixed"
			" with '%s' exceeds the max path length", unmodified_path, path_prefix);

	strcpy(temp_concatenated_string, path_prefix);
	strcat(temp_concatenated_string, unmodified_path);

	return temp_concatenated_string;
}

static inline FILE* open_file_safely(const char* const path, const char* const mode) {
	FILE* const file = fopen(get_temp_asset_path(path), mode);
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
