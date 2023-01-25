#include "utils/json.h"
#include "utils/safe_io.h" // For `read_file_contents`
#include "utils/failure.h" // For `FAIL`
#include "utils/alloc.h" // For `alloc`

////////// Some general fns

cJSON* init_json_from_file(const char* const path) {
	char* const contents = read_file_contents(path);
	cJSON* const json = cJSON_Parse(contents);

	if (json == NULL) FAIL(ParseJSON,
		"Cannot parse the JSON file with the path '%s'. Failing file portion:\n___\n%s\n___",
		path, cJSON_GetErrorPtr()
	);

	if (!cJSON_IsObject(json)) FAIL(ParseJSON,
		"Expected outermost JSON type from the file "
		"with the path '%s' to be an object", path
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

float get_float_from_json(const cJSON* const json) {
	if (!cJSON_IsNumber(json)) FAIL(ReadFromJSON, "Expected JSON object '%s' to be a number", json -> string);

	const double value = json -> valuedouble, min = (double) -FLT_MAX, max = (double) FLT_MAX;
	if (value < min || value > max) FAIL(ReadFromJSON, "Expected JSON object '%s' to be in the size range of [%f, %f]", min, max);
	return (float) value;
}

const char* get_string_from_json(const cJSON* const json) {
	if (!cJSON_IsString(json)) FAIL(ReadFromJSON, "Expected JSON object '%s' to be a string", json -> string);
	return json -> valuestring;
}

////////// Unsigned int primitive readers

static void check_size_of_unsigned_int(const int value, const uint16_t max) {
	if (value < 0 || value > max) FAIL(ReadFromJSON,
		"Expected number %d to be in the size range of [0, %llu]",
		value, max
	);
}

static int get_validated_json_unsigned_int(const cJSON* const json, const uint16_t max) {
	if (!cJSON_IsNumber(json)) FAIL(ReadFromJSON, "Expected JSON object to be a number", json -> string);

	const int value = json -> valueint;
	check_size_of_unsigned_int(value, max);
	return value;
}

uint8_t get_u8_from_json(const cJSON* const json) {
	return (uint8_t) get_validated_json_unsigned_int(json, UINT8_MAX);
}

uint16_t get_u16_from_json(const cJSON* const json) {
	return (uint16_t) get_validated_json_unsigned_int(json, UINT16_MAX);
}

////////// Array readers

// If the expected length is -1, the length isn't validated. The actual length is also returned.
static int validate_json_array(const cJSON* const json, const int expected_length) {
	const char* const array_name = json -> string;
	if (!cJSON_IsArray(json)) FAIL(ReadFromJSON, "Expected JSON object '%s' to be an array", array_name);

	const int length = cJSON_GetArraySize(json);

	if (length == 0) FAIL(ReadFromJSON, "%s", "JSON arrays cannot have size-zero lengths");

	else if ((expected_length != -1) && (length != expected_length)) FAIL(ReadFromJSON,
		"Expected JSON array '%s' to have a length of %d, but the length was %d",
		array_name, expected_length, length
	);

	return length;
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
JSON_ARRAY_READING_FN(uint16_t, u16)
JSON_ARRAY_READING_FN(float, float)

#undef JSON_ARRAY_READING_FN

////////// Vector readers

const char** make_string_vector_from_json(const cJSON* const json, uint8_t* const length) {
	const int int_length = validate_json_array(json, -1);
	check_size_of_unsigned_int(int_length, UINT8_MAX);

	*length = (uint8_t) int_length;
	const char** const strings = alloc(*length, sizeof(char*));

	const cJSON* string;
	int i = 0;

	cJSON_ArrayForEach(string, json)
		strings[i++] = get_string_from_json(string);

	return strings;
}

uint8_t* make_2D_map_from_json(const cJSON* const json, uint8_t size[2]) {
	const int map_height = validate_json_array(json, -1);
	check_size_of_unsigned_int(map_height, UINT8_MAX);

	//////////

	const cJSON* row;
	int i = 0, map_width;

	uint8_t *map, *curr_map_value;

	//////////

	cJSON_ArrayForEach(row, json) {
		if (i == 0) {
			map_width = validate_json_array(row, -1);
			check_size_of_unsigned_int(map_width, UINT8_MAX);

			map = alloc((size_t) (map_width * map_height), sizeof(uint8_t));
			curr_map_value = map;
		}

		// Checking that the other rows have the same length as the first one
		else validate_json_array(row, map_width);

		////////// Initializing row values

		const cJSON* height;
		cJSON_ArrayForEach(height, row)
			*(curr_map_value++) = get_u8_from_json(height);

		//////////

		i++;
	}

	size[0] = (uint8_t) map_width;
	size[1] = (uint8_t) map_height;

	return map;
}

//////////
