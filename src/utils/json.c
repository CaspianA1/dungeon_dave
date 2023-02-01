#include "utils/json.h"
#include "utils/safe_io.h" // For `read_file_contents`
#include "utils/failure.h" // For `FAIL`
#include "utils/alloc.h" // For `alloc`

////////// Some various internal utils

// TODO: to excluded
static const char* get_json_name(const cJSON* const json) {
	return json -> string;
}

static void typecheck_json(const cJSON* const json,
	cJSON_bool (*const type_checker) (const cJSON* const),
	const char* const expected_type) {

	/* TODO:
	- Make a fn for name getting
	- Print the actual type, instead of just the object */

	if (!type_checker(json)) {
		const char first = expected_type[0], *const indefinite_article =
			(first == 'a' || first == 'e' || first == 'i' || first == 'o' || first == 'u')
			? "an" : "a";

		FAIL(ReadFromJSON, "Expected JSON object '%s' to be %s %s. It looks like this: '%s'",
			get_json_name(json), indefinite_article, expected_type, cJSON_Print(json));
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
	const cJSON* const subobj = cJSON_GetObjectItem(json, key);

	if (subobj == NULL) FAIL(ReadFromJSON,
		"Could not find JSON key '%s' from object with name '%s'",
		key, json -> string
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

static int get_validated_json_unsigned_int(const cJSON* const json, const json_array_size_t max, const char* const name) {
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
		"Expected JSON array '%s' to have a length of %d, but the length was %d",
		get_json_name(json), expected_length, length
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

uint8_t* read_2D_map_from_json(const cJSON* const json, uint8_t size[2]) {
	const uint8_t max_size = UINT8_MAX;
	const uint8_t map_height = (uint8_t) validate_json_array(json, -1, max_size);

	uint8_t map_width, *map, *curr_map_value;

	//////////

	JSON_FOR_EACH(i, row, json,
		if (i == 0) {
			map_width = (uint8_t) validate_json_array(row, -1, max_size);
			map = alloc((size_t) (map_width * map_height), sizeof(uint8_t));
			curr_map_value = map;
		}

		// Checking that the other rows have the same length as the first one
		else validate_json_array(row, map_width, max_size);

		// Initializing row values
		JSON_FOR_EACH(_, height, row, *(curr_map_value++) = get_u8_from_json(height););
	);

	size[0] = (uint8_t) map_width;
	size[1] = (uint8_t) map_height;

	return map;
}

//////////
