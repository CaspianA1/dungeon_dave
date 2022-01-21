#include "level_parser.h"
#include "../list.c"

static FileContents read_file_contents(const char* const file_name) {
	FILE* const file = fopen(file_name, "r");
	if (file == NULL) FAIL(OpenFile, "could not open a file with the path of '%s'.", file_name);

	fseek(file, 0l, SEEK_END); // Set file position to end
	const long num_bytes = ftell(file);
	fseek(file, 0l, SEEK_SET); // Rewind file position

	char* const data = malloc(num_bytes + 1l);
	fread(data, num_bytes, 1, file); // Read file bytes
	data[num_bytes] = '\0';

	fclose(file);
	return (FileContents) {file_name, data, num_bytes};
}

static byte is_delim(const char curr_char, const char* delims) {
	while (*delims != '\0') {
		if (curr_char == *delims) return 1;
		delims++;
	}
	return 0;
}

static const char* tokenize(char* const to_tokenize, const char* const delims) {
	static char* buffer;
	if (to_tokenize != NULL) buffer = to_tokenize;

	while (is_delim(*buffer, delims)) *(buffer++) = '\0';

	const char* const start = buffer;
	if (*start == '\0') return NULL; // Whitespace followed by end of file

	while (!is_delim(*buffer, delims)) {
		if (*buffer == '\0') return start;
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
	*/

	const char* const delims = " \n\t\v\f\r";
	const char* token = tokenize(file_contents.data, delims);

	while (token != NULL) {
		push_ptr_to_list(&tokens, &token);
		token = tokenize(NULL, delims);
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

