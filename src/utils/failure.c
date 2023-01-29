#include "utils/failure.h"
#include <stdarg.h> // For variadic utils

// TODO: maybe print out a dialogue box, with `SDL_ShowSimpleMessageBox`
void print_failure_message(const char* const failure_type_string,
	const char* const format_string, const char* const filename, const int line_number, ...) {

	FILE* const out = stderr;

	fprintf(out, "Failed with error type '%s' in source file '%s' on line %d. Reason: '",
		failure_type_string, filename, line_number);
	
	va_list format_args;

	va_start(format_args, line_number);
	vfprintf(out, format_string, format_args);
	va_end(format_args);

	fputs("'.\n", out);
}
