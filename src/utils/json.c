#include "utils/json.h"
#include "utils/safe_io.h" // For `read_file_contents`
#include "utils/failure.h" // For `FAIL`
#include "utils/alloc.h" // For `alloc`, and `dealloc`
#include "data/constants.h" // For `max_map_size`
#include <limits.h> // For `CHAR_BIT`

////////// Some various internal utils

static const char* get_json_name(const cJSON* const json) {
	const char* const key_name = json -> string;
	return (key_name == NULL) ? "without key name" : key_name;
}

static void typecheck_json(const cJSON* const json,
	cJSON_bool (*const type_checker) (const cJSON* const),
	const char* const expected_type) {

	if (!type_checker(json)) {
		const char c = expected_type[0];
		const char* const indefinite_article_end = (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u') ? "n" : "";

		FAIL(ReadFromJSON, "Expected JSON object '%s', '%s', to be a%s %s", get_json_name(json),
			cJSON_Print(json), indefinite_article_end, expected_type);
	}
}

static void check_number_value_range(const cJSON* const json, const double max) {
	const double value = json -> valuedouble, min = 0.0;

	if (value < min || value > max) FAIL(ReadFromJSON,
		"Expected JSON object '%s' to be in the range of [%g, %g]",
		get_json_name(json), min, max
	);
}

////////// Some general fns

cJSON* init_json_from_file(const char* const path) {
	char* const contents = read_file_contents(path);
	cJSON* const json = cJSON_Parse(contents);

	if (json == NULL) FAIL(ParseJSON,
		"Cannot parse the JSON file with the path '%s'. Failing file portion:\n___\n%s\n___",
		path, cJSON_GetErrorPtr()
	);

	typecheck_json(json, cJSON_IsObject, "object");
	dealloc(contents);
	return json;
}

const cJSON* read_json_subobj(const cJSON* const json, const char* const key) {
	const cJSON* const subobj = cJSON_GetObjectItemCaseSensitive(json, key);

	if (subobj == NULL) FAIL(ReadFromJSON,
		"Could not find JSON key '%s' from object with name '%s'",
		key, get_json_name(json)
	);

	return subobj;
}

////////// Primitive readers (TODO: genericize)

bool get_bool_from_json(const cJSON* const json) {
	typecheck_json(json, cJSON_IsBool, "bool");
	return (bool) json -> valueint;
}

float get_float_from_json(const cJSON* const json) {
	typecheck_json(json, cJSON_IsNumber, "float");
	check_number_value_range(json, (double) FLT_MAX);
	return (float) json -> valuedouble;
}

const char* get_string_from_json(const cJSON* const json) {
	typecheck_json(json, cJSON_IsString, "string");
	return json -> valuestring;
}

////////// Unsigned int primitive readers

static int get_validated_json_unsigned_int(const cJSON* const json, const uint64_t max, const char* const name) {
	typecheck_json(json, cJSON_IsNumber, name);
	check_number_value_range(json, max);
	return json -> valueint;
}

#define JSON_UINT_READING_FN(num_bits) uint##num_bits##_t get_u##num_bits##_from_json(const cJSON* const json) {\
	return (uint##num_bits##_t) get_validated_json_unsigned_int(json, (uint##num_bits##_t) ~0u, "u" #num_bits);\
}

JSON_UINT_READING_FN(8)
JSON_UINT_READING_FN(16)

#undef JSON_UINT_READING_FN

////////// Array readers

// If the expected length is -1, the length isn't validated. The actual length is also returned.
json_array_size_t validate_json_array(const cJSON* const json, const int expected_length, const json_array_size_t max) {
	typecheck_json(json, cJSON_IsArray, "array");

	const int length = cJSON_GetArraySize(json);
	if (length == 0) FAIL(ReadFromJSON, "%s", "JSON arrays cannot have size-zero lengths");
	check_number_value_range(json, max);

	if ((expected_length != -1) && (length != expected_length)) FAIL(ReadFromJSON,
		"Expected JSON array '%s', '%s', to have a length of %d, but the length was %d",
		get_json_name(json), cJSON_Print(json), expected_length, length
	);

	return (json_array_size_t) length;
}

#define JSON_ARRAY_READING_FN(return_type_t, typename_t)\
	void read_##typename_t##s_from_json_array(const cJSON* const json, const json_array_size_t expected_length, return_type_t* const array) {\
		validate_json_array(json, expected_length, MAX_JSON_ARRAY_SIZE);\
		JSON_FOR_EACH(i, item, json, array[i] = get_##typename_t##_from_json(item););\
	}

JSON_ARRAY_READING_FN(uint8_t, u8)
JSON_ARRAY_READING_FN(uint16_t, u16)
JSON_ARRAY_READING_FN(float, float)

#undef JSON_ARRAY_READING_FN

////////// Vector readers

const char** read_string_vector_from_json(const cJSON* const json, json_array_size_t* const length) {
	*length = validate_json_array(json, -1, MAX_JSON_ARRAY_SIZE);
	const char** const strings = alloc(*length, sizeof(char*));

	JSON_FOR_EACH(i, string, json, strings[i] = get_string_from_json(string););

	return strings;
}

void* read_2D_map_from_json(const cJSON* const json, const buffer_size_t datum_size, map_pos_xz_t* const map_size) {
	////////// Computing the map height, and setting up some variables

	map_size -> z = (uint8_t) validate_json_array(json, -1, constants.max_map_size);

	map_pos_component_t map_width;
	void* map;
	uint8_t* curr_map_ptr;

	////////// Computing the max size for the datum

	const uint32_t num_datum_bits = datum_size * CHAR_BIT;
	const uint64_t max_size_for_datum = ((uint64_t) 1u << num_datum_bits) - 1u;

	////////// Looping over each row

	JSON_FOR_EACH(i, row, json,
		if (i == 0) {
			map_width = map_size -> x = (map_pos_component_t) validate_json_array(row, -1, constants.max_map_size);
			map = alloc((size_t) (map_width * map_size -> z), datum_size);
			curr_map_ptr = map;
		}

		// Checking that the other rows have the same length as the first one
		else validate_json_array(row, map_width, constants.max_map_size);

		// Initializing row values
		JSON_FOR_EACH(_, height, row,
			const uint64_t result = (uint64_t) get_validated_json_unsigned_int(height, max_size_for_datum, "a number");
			memcpy(curr_map_ptr, &result, datum_size);
			curr_map_ptr += datum_size;
		);
	);

	return map;
}
