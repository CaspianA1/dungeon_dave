#ifndef JSON_H
#define JSON_H

#include "cjson/cJSON.h" // For various JSON defs
#include <stdbool.h> // For `bool`
#include <stdint.h> // For sized ints

// Excluded: validate_json_array

////////// Some general fns

#define JSON_TO_FIELD(json, name, subtype_t) .name = get_##subtype_t##_from_json(read_json_subobj(json, #name))

cJSON* init_json_from_file(const char* const path);
#define deinit_json cJSON_Delete

cJSON* read_json_subobj(const cJSON* const json, const char* const key);

////////// Value readers

bool get_bool_from_json(const cJSON* const json);
uint8_t get_u8_from_json(const cJSON* const json);
uint16_t get_u16_from_json(const cJSON* const json);
float get_float_from_json(const cJSON* const json);
const char* get_string_from_json(const cJSON* const json);

#undef JSON_READING_DEF

////////// Array readers

#define JSON_ARRAY_READING_DEF(return_type_t, typename_t)\
	void read_##typename_t##s_from_json_array(const cJSON* const json, const int expected_length, return_type_t* const array);

JSON_ARRAY_READING_DEF(uint8_t, u8)
JSON_ARRAY_READING_DEF(float, float)

#undef JSON_ARRAY_READING_DEF

#endif
