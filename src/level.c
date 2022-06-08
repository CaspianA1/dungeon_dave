#ifndef LEVEL_C
#define LEVEL_C

#include "headers/level.h"
#include "headers/utils.h"
#include <cjson/cJSON.h>

/*
- Level in from a JSON file.
- Read into a LevelDescription struct. Input validation done there.
- General context created from that.
- Light params in the LevelDescription struct go to the GPU.

- Perhaps go directly from reading JSON to creating the level context?
*/

void level_test(void) {
	puts("Testing level system");

	char* const file_contents = read_file_contents(ASSET_PATH("levels/simple.json"));
	cJSON* const json = cJSON_Parse(file_contents);
	free(file_contents);

	char* const string_from_json = cJSON_Print(json);
	DEBUG(string_from_json, s);
	free(string_from_json);

	// Now, I need a way to grab expected fields from the json object, and fail if they aren't there

	cJSON_Delete(json);
}

#endif
