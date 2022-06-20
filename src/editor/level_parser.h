#include "editor.h"
#include "../headers/list.h"

#define QUOTE(a) #a
#define COLOR_STRING(color, format) "\033[" QUOTE(color) "m" format "\033[0m"

#define RED_COLOR 32

const struct {
	const char
		object_start, object_end,
		array_start, array_end,
		key_value_separator, element_delimiter,
		string_single_quote, string_double_quote;
} token_defs = {
	'{', '}', '[', ']', ':', ',', '\'', '\"'
};

// Later on, the token list can perhaps be one big string, just separated by '\0' symbols

typedef struct {
	const char* const name;
	char* const data;
	const size_t num_bytes;
} FileContents;

//////////

#define PARSE_FAIL(file_contents, message, contents_start, contents_end) do {\
	FAIL(ParseLevelFile, "Could not parse '%s' (%s):\n---\n" COLOR_STRING(RED_COLOR, "%.*s") "\n---",\
		(file_contents).name, (message), (int) ((contents_end) - (contents_start)),\
		file_contents.data + (contents_start));\
} while (false)

////////// Excluded: read_file_contents, is_delim, tokenize, lex_json_file, parse_json_file

// Later on, init_level_state_from_json_file
void init_editor_state_from_json_file(EditorState* const eds, const char* const file_name);
