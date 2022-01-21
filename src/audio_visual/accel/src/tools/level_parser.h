#include "editor.h"
#include "../headers/list.h"

typedef struct {
	const char* const file_name;
	char* const data;
} FileContents;

////////// Excluded: read_file_contents, lex_json_file, parse_json_file, 

// Later on, init_level_state_from_json_file
void init_editor_state_from_json_file(EditorState* const eds, const char* const file_name);
