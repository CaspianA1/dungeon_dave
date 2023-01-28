#ifndef JSON_H
#define JSON_H

#include "cjson/cJSON.h" // For various JSON defs
#include "utils/macro_utils.h" // For `ARRAY_LENGTH`
#include <stdbool.h> // For `bool`
#include <stdint.h> // For sized ints

// Excluded: check_size_of_unsigned_int, get_validated_json_unsigned_int, validate_json_array

////////// Some general fns

#define JSON_TO_FIELD(json, name, subtype_t) .name = get_##subtype_t##_from_json(read_json_subobj(json, #name))
#define GET_ARRAY_VALUES_FROM_JSON_KEY(json, c_name, json_name, subtype_t) read_##subtype_t##s_from_json_array(read_json_subobj(json, #json_name), ARRAY_LENGTH(c_name), c_name)

#define JSON_FOR_EACH(index_var, loop_item, json, ...) do {\
	uint16_t index_var = 0;\
	const cJSON* loop_item;\
	cJSON_ArrayForEach(loop_item, json) {\
		__VA_ARGS__\
		index_var++;\
	}\
} while (false)

cJSON* init_json_from_file(const char* const path);
#define deinit_json cJSON_Delete

cJSON* read_json_subobj(const cJSON* const json, const char* const key);

////////// Primitive readers

bool get_bool_from_json(const cJSON* const json);
float get_float_from_json(const cJSON* const json);
const char* get_string_from_json(const cJSON* const json);

void check_size_of_unsigned_int(const int value, const uint16_t max);

#define JSON_UNSIGNED_INT_READING_DEF(num_bits) uint##num_bits##_t get_u##num_bits##_from_json(const cJSON* const json);

JSON_UNSIGNED_INT_READING_DEF(8)
JSON_UNSIGNED_INT_READING_DEF(16)

#undef JSON_UNSIGNED_INT_READING_DEF

////////// Array readers

int validate_json_array(const cJSON* const json, const int expected_length);

#define JSON_ARRAY_READING_DEF(return_type_t, typename_t)\
	void read_##typename_t##s_from_json_array(const cJSON* const json, const int expected_length, return_type_t* const array);

JSON_ARRAY_READING_DEF(uint8_t, u8)
JSON_ARRAY_READING_DEF(uint16_t, u16)
JSON_ARRAY_READING_DEF(float, float)

#undef JSON_ARRAY_READING_DEF

#endif

////////// Vector readers

// Note: the strings in this array belong to the input JSON.
const char** read_string_vector_from_json(const cJSON* const json, uint16_t* const size);

// Note: the returned map should be freed via `dealloc`. `size` is an output variable as well.
uint8_t* read_2D_map_from_json(const cJSON* const json, uint8_t size[2]);
