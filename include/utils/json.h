#ifndef JSON_H
#define JSON_H

#include "cjson/cJSON.h" // For various JSON defs
#include <stdbool.h> // For `bool`, and `false`
#include "utils/typedefs.h" // For `map_pos_xz_t`, and `map_pos_component_t`

typedef uint16_t json_array_size_t;
#define MAX_JSON_ARRAY_SIZE UINT16_MAX

/* Excluded: get_json_name, typecheck_json, check_number_value_range_with_options,
check_number_value_range, get_validated_json_unsigned_int, validate_json_array */

////////// Some useful macros

#define WITH_JSON_OBJ_SUFFIX(json_obj) json_obj##_json
#define JSON_OBJ_NAME_DEF(json_obj) *const WITH_JSON_OBJ_SUFFIX(json_obj)
#define READ_JSON_SUBOBJ_WITH_SUFFIX(json_obj, json_subobj) read_json_subobj(WITH_JSON_OBJ_SUFFIX(json_obj), #json_subobj)

#define JSON_TO_FIELD(json_obj, json_subobj, subtype_t)\
	.json_subobj = get_##subtype_t##_from_json(\
	READ_JSON_SUBOBJ_WITH_SUFFIX(json_obj, json_subobj))

#define DEF_ARRAY_FROM_JSON(json_obj, json_subobj, actual_subtype_t, aliased_subtype_t, array_length)\
	actual_subtype_t json_obj##_##json_subobj[array_length];\
	\
	read_##aliased_subtype_t##s_from_json_array(\
		READ_JSON_SUBOBJ_WITH_SUFFIX(json_obj, json_subobj),\
		array_length, json_obj##_##json_subobj)

#define DEF_JSON_SUBOBJ(json_obj, json_subobj)\
	JSON_OBJ_NAME_DEF(json_subobj) =\
	READ_JSON_SUBOBJ_WITH_SUFFIX(json_obj, json_subobj)

#define EXTRACT_FROM_JSON_SUBOBJ(extracting_fn, json_obj, json_subobj, ...)\
	json_subobj = extracting_fn##_from_json(READ_JSON_SUBOBJ_WITH_SUFFIX(json_obj, json_subobj) __VA_ARGS__)

// This does not change any suffix names, whereas `JSON_FOR_EACH` does.
#define RAW_JSON_FOR_EACH(index_var, loop_item, json_obj, ...) do {\
	json_array_size_t index_var = 0;\
	const cJSON* loop_item;\
	cJSON_ArrayForEach(loop_item, json_obj) {\
		__VA_ARGS__\
		index_var++;\
	}\
} while (false)

#define JSON_FOR_EACH(index_var, loop_item, json_obj, ...)\
	RAW_JSON_FOR_EACH(index_var, WITH_JSON_OBJ_SUFFIX(loop_item),\
	WITH_JSON_OBJ_SUFFIX(json_obj), __VA_ARGS__)

////////// Some general fns

cJSON* init_json_from_file(const char* const path);
#define deinit_json cJSON_Delete

const cJSON* read_json_subobj(const cJSON* const json, const char* const key);

////////// Primitive readers

bool get_bool_from_json(const cJSON* const json);
float get_float_from_json(const cJSON* const json);
const char* get_string_from_json(const cJSON* const json);

#define JSON_UINT_READING_DEF(num_bits) uint##num_bits##_t get_u##num_bits##_from_json(const cJSON* const json);

JSON_UINT_READING_DEF(8)
JSON_UINT_READING_DEF(16)

#undef JSON_UINT_READING_DEF

////////// Array readers

json_array_size_t validate_json_array(const cJSON* const json, const int expected_length, const json_array_size_t max);

#define JSON_ARRAY_READING_DEF(return_type_t, typename_t) void read_##typename_t##s_from_json_array(\
	const cJSON* const json, const json_array_size_t expected_length, return_type_t* const array);

JSON_ARRAY_READING_DEF(uint8_t, u8)
JSON_ARRAY_READING_DEF(uint16_t, u16)
JSON_ARRAY_READING_DEF(float, float)
JSON_ARRAY_READING_DEF(float, possibly_negative_float)

#undef JSON_ARRAY_READING_DEF

#endif

////////// Vector readers

// Note: the returned vectors should be freed via `dealloc`. `length` and `map_size` are output variables as well.

// Note: the strings in this array belong to the input JSON.
const char** read_string_vector_from_json(const cJSON* const json, json_array_size_t* const length);

// Assuming that the datums are numbers that have a power-of-two number of bits.
void* read_2D_map_from_json(const cJSON* const json, const buffer_size_t datum_size, map_pos_xz_t* const map_size);
