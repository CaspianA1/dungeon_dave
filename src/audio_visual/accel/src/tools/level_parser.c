#include "level_parser.h"
#include "../list.c"

static FileContents read_file_contents(const char* const file_name) {
	FILE* const file = fopen(file_name, "r");
	if (file == NULL) FAIL(OpenFile, "could not open a file with the path of '%s'.", file_name);

	fseek(file, 0l, SEEK_END); // Set file position to end
	const size_t num_bytes = (size_t) ftell(file);
	fseek(file, 0l, SEEK_SET); // Rewind file position

	char* const data = malloc(num_bytes + 1l);
	fread(data, num_bytes, 1, file); // Read file bytes
	data[num_bytes] = '\0';

	fclose(file);
	return (FileContents) {file_name, data, num_bytes};
}

/*
static byte char_in_set(const char curr_char, const char* set) {
	while (*set!= '\0') {
		if (curr_char == *set) return 1;
		set++;
	}
	return 0;
}
*/

static const char* tokenize(char* const to_tokenize) {
	static char* buffer;
	if (to_tokenize != NULL) buffer = to_tokenize;

	// While the buffer has whitespace, progress its ptr
	while (isspace(*buffer)) buffer++;

	const char* const start = buffer, first = *buffer;
	if (first == '\0') return NULL; // Whitespace followed by end of file

	// If the beginning of a string is detected, read until the end of the string and return that token
	else if (first == token_defs.string_single_quote || first == token_defs.string_double_quote) {
		for (char c = *(++buffer); c != '\0' && c != first; c = *(buffer++));

		if (*buffer != '\0') *(buffer++) = '\0';
		return start;
	}

	while (!isspace(*buffer)) {
		const char c = *buffer;
		if (c == '\0') return start;

		buffer++;
	}

	*(buffer++) = '\0';
	return start;
}

// Returns a list of all tokens
static List lex_json_file(const FileContents file_contents) {
	List tokens = init_list(1, char*);

	/*
	- Split on {}[]:,'" while still keeping them
	- Treat everything inside a string as 1 token

	- Preprocess the file contents like this:

	Padding character: one space
	Pad the {}[]:, characters on the left and the right
	Pad the left side of string openings, and the right side of string closings

	- Later on, after a token has been extracted, categorize it into a token type (that will make token comparison faster)
	*/

	const char* token = tokenize(file_contents.data);

	while (token != NULL) {
		push_ptr_to_list(&tokens, &token);
		token = tokenize(NULL);
	}

	/*
	for (buffer_size_t i = 0; i < tokens.length; i++) {
		const char* const token = ((char**) (tokens.data))[i];
		printf("token = '%s'\n", token);
	}
	*/

	return tokens;
}

// Sets num_textures, map_size, heightmap, texture_id_map, map_name, renderer, and textures
static void parse_json_file(EditorState* const eds, FileContents* const file_contents) {
	puts("About to parse json file");
	// printf("Parse this: '%s'\n", file_contents -> data);
	(void) eds;

	/*
	1. Tokenize
	2. Parse into tree
	*/

	const List tokens = lex_json_file(*file_contents);
	deinit_list(tokens);
}

void init_editor_state_from_json_file(EditorState* const eds, const char* const file_name) {
	FileContents file_contents = read_file_contents(file_name);
	parse_json_file(eds, &file_contents);
	free(file_contents.data);
}
