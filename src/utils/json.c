#include "utils/json.h"
#include "utils/safe_io.h" // For `read_file_contents`
#include "utils/failure.h" // For `FAIL`

////////// Some general fns

cJSON* init_json_from_file(const char* const path) {
	char* const contents = read_file_contents(path);
	cJSON* const json = cJSON_Parse(contents);

	if (json == NULL) FAIL(ParseJSON,
		"Cannot parse the JSON file with the path '%s'. Failing file portion:\n___\n%s\n___",
		path, cJSON_GetErrorPtr()
	);

	dealloc(contents);
	return json;
}

cJSON* read_json_subobj(const cJSON* const json, const char* const key) {
	cJSON* const subobj = cJSON_GetObjectItem(json, key);

	if (subobj == NULL) FAIL(ReadFromJSON,
		"Could not find JSON key '%s' from object with name '%s'",
		key, json -> string
	);

	return subobj;
}

////////// Primitive readers (TODO: genericize)

bool get_bool_from_json(const cJSON* const json) {
	if (!cJSON_IsBool(json)) FAIL(ReadFromJSON, "Expected JSON object '%s' to be a bool", json -> string);
	return (bool) json -> valueint;
}

uint8_t get_u8_from_json(const cJSON* const json) {
	if (!cJSON_IsNumber(json)) FAIL(ReadFromJSON, "Expected JSON object '%s' to be a number", json -> string);

	const int value = json -> valueint;
	if (value < 0 || value > UINT8_MAX) FAIL(ReadFromJSON, "Expected JSON object '%s' to be in the size range of [0, %hhu]", UINT8_MAX);
	return (uint8_t) value;
}

float get_float_from_json(const cJSON* const json) {
	if (!cJSON_IsNumber(json)) FAIL(ReadFromJSON, "Expected JSON object '%s' to be a number", json -> string);

	const double value = json -> valuedouble, min = (double) -FLT_MAX, max = (double) FLT_MAX;
	if (value < min || value > max) FAIL(ReadFromJSON, "Expected JSON object '%s' to be in the size range of [%f, %f]", min, max);
	return (float) value;
}

const char* get_string_from_json(const cJSON* const json) {
	if (!cJSON_IsString(json)) FAIL(ReadFromJSON, "JSON key '%s' expected to be a string", json -> string);
	return json -> valuestring;
}

////////// Array readers

static void validate_json_array(const cJSON* const json, const int expected_length) {
	const char* const array_name = json -> string;
	if (!cJSON_IsArray(json)) FAIL(ReadFromJSON, "Expected JSON object '%s' to be an array", array_name);

	const int length = cJSON_GetArraySize(json);

	if (length != expected_length) FAIL(ReadFromJSON,
		"Expected JSON array '%s' to have a length of %d, but the length was %d",
		array_name, expected_length, length
	);
}

#define JSON_ARRAY_READING_FN(return_type_t, typename_t)\
	void read_##typename_t##s_from_json_array(const cJSON* const json, const int expected_length, return_type_t* const array) {\
		validate_json_array(json, expected_length);\
		\
		int index = 0;\
		const cJSON* item;\
		\
		cJSON_ArrayForEach(item, json) array[index++] = get_##typename_t##_from_json(item);\
	}

JSON_ARRAY_READING_FN(uint8_t, u8)
JSON_ARRAY_READING_FN(float, float)

#undef JSON_ARRAY_READING_FN

//////////
