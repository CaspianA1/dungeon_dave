#include "ddl_parser.h"

// The pointer in the file contents returned by this function should be freed by the caller
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
	return (FileContents) {file_name, data};
}

SECTION_PARSER_DEF(name) {
	(void) curr_token;
	(void) file_name;
	(void) delims;
	(void) eds;
	puts("Name parser");
	return NEXT_PARSER_TOKEN();
}

SECTION_PARSER_DEF(map_size) {
	(void) curr_token;
	(void) file_name;
	(void) delims;
	(void) eds;
	puts("Map size parser");

	return NEXT_PARSER_TOKEN();
}

SECTION_PARSER_DEF(heightmap) {
	(void) curr_token;
	(void) file_name;
	(void) delims;
	(void) eds;
	puts("Heightmap parser");

	const byte expected_width = 8, expected_height = 7;
	// byte* const map = malloc(expected_width * expected_height * sizeof(byte));

	for (byte y = 0; y < expected_height; y++) {
		for (byte x = 0; x < expected_width; x++, curr_token = NEXT_PARSER_TOKEN()) {
			if (!VARARG_SECTION_PARSER_HAS_TOKEN()) // May not be needed later on
				FAIL(ParseLevelFile, "Ran out of tokens for heightmap in '%s'.", file_name);

			const byte num_chars = strlen(curr_token);
			if (num_chars > 3)
				FAIL(ParseLevelFile, "Token is too long for heightmap in '%s'.", file_name);

			for (byte i = 0; i < num_chars; i++) {
				const char c = curr_token[i];
				// printf("c = '%c'\n", c);
				if (c < '0' || c > '9')
					FAIL(ParseLevelFile, "Expected numerical token for heightmap in '%s'.", file_name);
			}
		}
	}

	return curr_token;
}

SECTION_PARSER_DEF(texture_id_map) {
	(void) curr_token;
	(void) file_name;
	(void) delims;
	(void) eds;
	puts("Texture id map parser");

	while (VARARG_SECTION_PARSER_HAS_TOKEN()) {
		// DEBUG(curr_token, s);
		curr_token = NEXT_PARSER_TOKEN();
	}
	return curr_token;
}

SECTION_PARSER_DEF(wall_texture) {
	(void) curr_token;
	(void) file_name;
	(void) delims;
	(void) eds;
	puts("Wall texture parser");

	while (VARARG_SECTION_PARSER_HAS_TOKEN()) {
		// DEBUG(curr_token, s);
		curr_token = NEXT_PARSER_TOKEN();
	}
	return curr_token;
}

static void remove_comments_from_file_data(char* data) {
	while (*data != '\0') {
		if (*data == COMMENT_START) { // If found a comment, add spaces until the end of the line or file
			while ((*data != '\0') && (*data != '\n')) *(data++) = ' ';
		}
		if (*data != '\0') data++;
	}
}

static void parse_ddl_file(EditorState* const eds, FileContents* const file_contents) {
	char* const data = file_contents -> data;
	remove_comments_from_file_data(data);

	const char
		*const delims = " \n\t\v\f\r",
		*const file_name = file_contents -> file_name,
		*token = INITIAL_PARSER_TOKEN(data, delims);

	while (token != NULL) {
		if (token[0] == SECTION_TAG_START) {
			const char* const tag_name = token + 1;
			const SectionTag* matching_tag = NULL;

			for (byte i = 0; i < num_section_tag_types; i++) {
				const SectionTag* const cmp_tag = section_tags + i;
				const char* const cmp_tag_name = cmp_tag -> name;
				if (strncmp(cmp_tag_name, tag_name, strlen(cmp_tag_name)) == 0) {
					matching_tag = cmp_tag;
					break;
				}
			}

			if (matching_tag == NULL)
				FAIL(ParseLevelFile, "invalid section tag, '%s', for '%s'.", tag_name, file_name);

			const char* const first_arg = NEXT_PARSER_TOKEN();
			if ((first_arg == NULL) || (first_arg[0] == SECTION_TAG_START))
				FAIL(ParseLevelFile, "the section tag '%s' in '%s' does not have any arguments.", tag_name, file_name);

			token = matching_tag -> section_parser(first_arg, delims, file_name, eds);
		}
		else FAIL(ParseLevelFile, "invalid token in '%s': '%s'.", file_name, token);
	}
}

void init_editor_state_from_ddl_file(EditorState* const eds, const char* const file_name) {
	FileContents file_contents = read_file_contents(file_name);
	parse_ddl_file(eds, &file_contents);
	free(file_contents.data);
}
