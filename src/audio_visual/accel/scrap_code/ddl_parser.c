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
	(void) delims;
	(void) file_name;
	(void) eds;

	puts("Name parser");
	*parse_status_bits |= BIT_PARSED_NAME;
	return NEXT_PARSER_TOKEN();
}

SECTION_PARSER_DEF(map_size) {
	(void) curr_token;
	(void) delims;
	(void) file_name;
	(void) eds;

	puts("Map size parser");
	*parse_status_bits |= BIT_PARSED_MAP_SIZE;
	return NEXT_PARSER_TOKEN();
}

SECTION_PARSER_DEF(wall_texture) {
	(void) curr_token;
	(void) delims;
	(void) file_name;
	(void) eds;

	puts("Wall texture parser");

	while (VARARG_SECTION_PARSER_HAS_TOKEN()) {
		// DEBUG(curr_token, s);
		curr_token = NEXT_PARSER_TOKEN();
	}

	*parse_status_bits |= BIT_PARSED_WALL_TEXTURES;
	return curr_token;
}

// This parses heightmaps and texture id maps
SECTION_PARSER_DEF(any_map) {
	(void) parse_status_bits;
	(void) eds;
	puts("Any map parser");

	const char* const map_type = curr_token;

	const byte
		is_heightmap = strcmp(heightmap_tag_name, map_type) == 0,
		is_texture_id_map = strcmp(texture_id_map_tag_name, map_type) == 0;

	if (!is_heightmap && !is_texture_id_map)
		FAIL(ParseLevelFile, "unrecognized map type in '%s': %s.", file_name, map_type);
	else if (!(*parse_status_bits & BIT_PARSED_MAP_SIZE))
		FAIL(ParseLevelFile, "expected map size section to come before map section in '%s'.", file_name);
	else if (is_texture_id_map && !(*parse_status_bits & BIT_PARSED_WALL_TEXTURES))
		FAIL(ParseLevelFile, "expected texture section to come before texture id map section in '%s'.", file_name);

	curr_token = NEXT_PARSER_TOKEN();

	const byte expected_width = 8, expected_height = 7;
	// byte* const map = malloc(expected_width * expected_height * sizeof(byte));

	for (byte y = 0; y < expected_height; y++) {
		for (byte x = 0; x < expected_width; x++, curr_token = NEXT_PARSER_TOKEN()) {
			if (!VARARG_SECTION_PARSER_HAS_TOKEN()) // May not be needed later on
				FAIL(ParseLevelFile, "ran out of tokens for %s map in '%s'.", map_type, file_name);

			const byte num_chars = strlen(curr_token);
			if (num_chars > 3) FAIL(ParseLevelFile, "token is too long for %s map in '%s'.", map_type, file_name);

			for (byte i = 0; i < num_chars; i++) {
				const char c = curr_token[i];
				if (c < '0' || c > '9') FAIL(ParseLevelFile, "expected numerical token for %s map in '%s'.", map_type, file_name);
			}

			const int16_t map_value = three_chars_to_int(curr_token, num_chars);
			if (map_value > 255) FAIL(ParseLevelFile, "%s map value in '%s', '%d', "
				"exceeds the maximum byte size.", map_type, file_name, map_value);

			// DEBUG(map_value, d);
			/* If a texture id map, max is 32. Check that if texture id map, doesn't exceed max num textures;
			but texture num validation should happen before texture id map section. */
		}
	}

	*parse_status_bits |= is_heightmap ? BIT_PARSED_HEIGHTMAP : BIT_PARSED_TEXTURE_ID_MAP;
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

	byte parse_status_bits = 0;

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

			token = matching_tag -> section_parser(&parse_status_bits, first_arg, delims, file_name, eds);
		}
		else FAIL(ParseLevelFile, "invalid token in '%s': '%s'.", file_name, token);
	}

	if (parse_status_bits != BITS_PARSED_ALL_MAP_SECTIONS)
		FAIL(ParseLevelFile, "Did not include all 5 required sections in '%s'.", file_name);
}

void init_editor_state_from_ddl_file(EditorState* const eds, const char* const file_name) {
	FileContents file_contents = read_file_contents(file_name);
	parse_ddl_file(eds, &file_contents); // Sets num_textures, map_size, heightmap, texture_id_map, map_name, textures
	free(file_contents.data);

	/* This should be freed:
	heightmap, texture_id_map, and map_name, via free
	textures, via SDL_DestroyTexture
	renderer, via SDL_DestroyRenderer */
}
