#ifndef MACRO_UTILS_H
#define MACRO_UTILS_H

#include <stdbool.h> // For `bool`, `true`, and `false`

#define CHECK_BITMASK(bits, mask) (!!((bits) & (mask)))
#define ARRAY_LENGTH(array) (sizeof((array)) / sizeof(*(array)))

#define ON_FIRST_CALL(...) do {\
	static bool first_call = true;\
	if (first_call) {\
		__VA_ARGS__\
		first_call = false;\
	}\
} while (false)

#endif
