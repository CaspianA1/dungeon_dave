#ifndef SAFE_IO_H
#define SAFE_IO_H

#include <stdarg.h> // For variadic utils
#include <stdio.h> // For various IO-related functions
#include "utils/failure.h" // For `FAIL`
#include "utils/alloc.h" // For `alloc`

// TODO: put in `constants.h`?
static const char* const ASSET_PATH_PREFIX = "../../assets/";

// The returned string should be freed with `dealloc`. TODO: put this in a better place
static inline char* make_formatted_string(const char* const format, ...) {
	va_list args;
	va_start(args, format);
	const size_t size = (size_t) vsnprintf(NULL, 0, format, args) + 1;
	va_end(args);

	char* const formatted_string = alloc(size, sizeof(char));

	va_start(args, format);
	vsnprintf(formatted_string, size, format, args);
	va_end(args);

	return formatted_string;
}

/* Why this is used:
- All assets in the `assets` directory share the common prefix defined in this function.
Therefore, it's wasteful to repeat this string across every asset used, and if the path changes
for whatever reason, then one would have to manually change all of the path prefixes.

- This function concatenates the input path with the path prefix, returning a static buffer
that must be fully used before the function is called again. Therefore, it is encouraged
to call this function as late as possible, in terms of when the asset path is needed. */
static inline const char* get_temp_asset_path(const char* const unmodified_path) {
	enum {max_concatenation_buffer_size = 100u}; // TODO: put in `constants.h`?

	static char temp_concatenated_string[max_concatenation_buffer_size];

	const size_t
		asset_path_prefix_length = strlen(ASSET_PATH_PREFIX),
		unmodified_path_length = strlen(unmodified_path);

	// Adding 1 at the end for the null terminator
	if (asset_path_prefix_length + unmodified_path_length + 1 > max_concatenation_buffer_size)
		FAIL(OpenFile, "Cannot open the file with path '%s', since its path when prefixed"
			" with '%s' exceeds the max path length", unmodified_path, ASSET_PATH_PREFIX);

	memcpy(temp_concatenated_string, ASSET_PATH_PREFIX, asset_path_prefix_length);
	memcpy(temp_concatenated_string + asset_path_prefix_length, unmodified_path, unmodified_path_length + 1);

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
