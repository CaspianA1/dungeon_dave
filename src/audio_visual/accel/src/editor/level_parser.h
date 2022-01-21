#include "editor.h"

typedef struct {
	const char* const file_name;
	char* const data;
} FileContents;

////////// Excluded: read_file_contents, parse_json_file

// Later on, init_level_state_from_json_file
void init_editor_state_from_json_file(EditorState* const eds, const char* const file_name);
