#ifndef LEVEL_FILE_C
#define LEVEL_FILE_C

#include "headers/level_file.h"
#include "headers/utils.h"
#include <cjson/cJSON.h>

typedef enum {
	Equivalent,
	MismatchingTypes,
	DifferingKeyCount,
	MismatchingKeyNames
} StructuralEquivalenceState;

/*
- Level in from a JSON file.
- Read into a LevelDescription struct. Input validation done there.
- General context created from that.
- Light params in the LevelDescription struct go to the GPU.

- Perhaps go directly from reading JSON to creating the level context?
*/

//////////

// This exists for debugging. Once done, TODO: remove.
static const char* get_json_type_string(const int type) {
	#define TYPE_CASE(type, string) case type: return string

	switch (type) {
		TYPE_CASE(cJSON_Invalid, "invalid");
		TYPE_CASE(cJSON_False, "false");
		TYPE_CASE(cJSON_True, "true");
		TYPE_CASE(cJSON_NULL, "null");
		TYPE_CASE(cJSON_Number, "number");
		TYPE_CASE(cJSON_String, "string");
		TYPE_CASE(cJSON_Array, "array");
		TYPE_CASE(cJSON_Object, "object");
		TYPE_CASE(cJSON_Raw, "raw");
		default: return "unrecognized";
	}

	#undef TYPE_CASE
}

// This is also for debugging. TODO: remove.
static void print_json(const cJSON* const json) {
	char* const string = cJSON_Print(json);
	puts(string);
	free(string);
}

static cJSON* init_json_from_path(const char* const path) {
	char* const file_contents = read_file_contents(path);
	cJSON* const json = cJSON_Parse(file_contents);

	if (json == NULL) {
		const char* const library_error_divider = "----------";
		FAIL(ParseLevelFile, "Could not parse level file with path %s. Failed here: \n%s\n%s%s\n",
			path, library_error_divider, cJSON_GetErrorPtr(), library_error_divider);
	}

	free(file_contents);
	return json;
}

///////////

static StructuralEquivalenceState check_json_structural_equivalence(const cJSON* const a, const cJSON* const b) {
	const int type_a = a -> type, type_b = b -> type;

	if (type_a != type_b) {
		const bool
			type_a_is_bool = (type_a == cJSON_False || type_a == cJSON_True),
			type_b_is_bool = (type_b == cJSON_False || type_b == cJSON_True);
		
		// Bools should be counted as the same type, even though the library differentiates them
		return type_a_is_bool && type_b_is_bool ? Equivalent : MismatchingTypes;
	}

	else if (type_a == cJSON_Array) {
		// e = element. Checking structural equivalence at corresponding indices, stopping if either runs out of elements.
		for (const cJSON *ea = a -> child, *eb = b -> child; ea != NULL && eb != NULL; ea = ea -> next, eb = eb -> next) {
			if (check_json_structural_equivalence(ea, eb) != Equivalent) return MismatchingTypes;
		}
	}

	else if (type_a == cJSON_Object) {
		// Checking that each has the same set of keys in the same order, and the structural equivalence of all values.
		for (const cJSON *kv_a = a -> child, *kv_b = b -> child;;
			kv_a = kv_a -> next, kv_b = kv_b -> next) { // kv = key value

			const bool end_of_a = kv_a == NULL, end_of_b = kv_b == NULL;
			if (end_of_a && end_of_b) break; // End of both objects

			// End of one, but not the other
			if (end_of_a ^ end_of_b) return DifferingKeyCount;
			// End of neither here
			else if (strcmp(kv_a -> string, kv_b -> string)) return MismatchingKeyNames;
			// Checking equivalence in values of keys
			else if (check_json_structural_equivalence(kv_a, kv_b) != Equivalent) return MismatchingTypes;
		}
	}

	return type_a == type_b ? Equivalent : MismatchingTypes; // Must be a primitive here
}

///////////

void level_file_test(void) {
	cJSON
		*const json = init_json_from_path(ASSET_PATH("levels/level_one.json")),
		*const expected = init_json_from_path(ASSET_PATH("levels/level_layout.json"));

	const char* status;

	#define EQUIVALENCE_CASE(enum_member, string) case enum_member: status = string; break;

	switch (check_json_structural_equivalence(json, expected)) {
		EQUIVALENCE_CASE(Equivalent, "Equivalent");
		EQUIVALENCE_CASE(MismatchingTypes, "Types don't match");
		EQUIVALENCE_CASE(DifferingKeyCount, "Key count is not the same");
		EQUIVALENCE_CASE(MismatchingKeyNames, "Key names are not the same");
	}

	printf("status = '%s'\n", status);

	#undef EQUIVALENCE_CASE

	// Now, I need a way to grab expected fields from the json object, and fail if they aren't there

	(void) get_json_type_string;
	(void) print_json;

	cJSON_Delete(expected);
	cJSON_Delete(json);
}

#endif
