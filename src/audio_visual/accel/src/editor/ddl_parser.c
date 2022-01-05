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
	return (FileContents) {file_name, data, num_bytes};
}

static void progress_char_index_to_tag_argument(long* const char_index_ref,
	const SectionTag* const tag, const FileContents* const file_contents) {

	long char_index = *char_index_ref + strlen(tag -> name) + 1;
	const long orig_char_index = char_index;

	const char* const data = file_contents -> data;

	while ((char_index < file_contents -> num_bytes) && isspace(data[char_index])) char_index++;

	if ((char_index == orig_char_index) || (data[char_index] == SECTION_TAG_START))
		FAIL(ParseLevelFile, "a level tag in '%s' needs an argument.", file_contents -> file_name);

	*char_index_ref = char_index - 1;
}

static const SectionTag* get_section_tag(const long char_index, const FileContents* const file_contents) {
	/* While not reached space or newline, collect tag name
	Curr tag types: name, heightmap, texture_id_map, wall_textures 
	Also, a variant of one-line tags
	Goal is to get correct section tag out, by comparing the string up until a newline (do the multiline variant first)
	Need whitespace after a one-line tag */

	// No need to worry about overflow here since strncmp accounts for a possible null terminator
	const char* const tag_start = file_contents -> data + char_index + 1;
	const SectionTag* matching_tag = NULL;

	for (byte i = 0; i < num_section_tag_types; i++) {
		const SectionTag* const tag_ref = section_tags + i;
		const char* const tag_name = tag_ref -> name;

		const size_t tag_length = strlen(tag_name);

		// If the tag matches, and there's a whitespace character after it
		if (strncmp(tag_name, tag_start, tag_length) == 0 && isspace(tag_start[tag_length])) {
			matching_tag = tag_ref;
			break;
		}
	}

	if (matching_tag == NULL)
		FAIL(ParseLevelFile, "did not recognize a level tag for '%s'.", file_contents -> file_name);

	// else printf("Found a tag match: '%s'\n", matching_tag -> name);
	return matching_tag;
}

SECTION_PARSER_DEF(name) {
	(void) start_char_index;
	(void) end_char_index;
	(void) file_contents;
	(void) eds;
	puts("Name parser");

	// strtok?

	/*
	const size_t name_length = end_char_index - start_char_index + 1;
	DEBUG(name_length, zu);
	*/

	/*
	file_contents -> data[end_char_index + 1] = '\0';
	printf("Name: '%s'\n", file_contents -> data + start_char_index);
	*/
}

SECTION_PARSER_DEF(heightmap) {
	(void) start_char_index;
	(void) end_char_index;
	(void) file_contents;
	(void) eds;
	puts("Heightmap parser");
}

SECTION_PARSER_DEF(texture_id_map) {
	(void) start_char_index;
	(void) end_char_index;
	(void) file_contents;
	(void) eds;
	puts("Texture id map parser");
}

SECTION_PARSER_DEF(wall_texture) {
	(void) start_char_index;
	(void) end_char_index;
	(void) file_contents;
	(void) eds;
	puts("Wall texture parser");
}

// The beginning of a section is marked by an ampersand an an identifier, and then an argument
static void parse_section(long* const char_index_ref, const FileContents* const file_contents,
	const SectionTag* const tag, const EditorState* const eds) {

	const char* const data = file_contents -> data;

	long char_index = *char_index_ref + 1;
	const long arg_start_index = char_index;

	// Find start of next argument, at its ampersand; and after that, backtrack, ignoring all whitespace characters
	while ((char_index < file_contents -> num_bytes) && (data[char_index] != SECTION_TAG_START)) char_index++;
	while (isspace(data[--char_index]));

	*char_index_ref = char_index; // Here, char_index equals arg_end_index
	tag -> section_parser(arg_start_index, char_index, file_contents, eds);
	
	// printf("Start char is '%c', and end char is '%c'\n|\n", data[arg_start_index], data[char_index]);

	/* data[char_index + 1] = '\0';
	printf("Arg: '%s'\n", data + arg_start_index); */
}

static void parse_ddl_file(EditorState* const eds, FileContents* const file_contents) {
	char* const data = file_contents -> data;
	const long num_bytes = file_contents -> num_bytes;

	// This erases all comments and replaces them with whitespace
	for (long i = 0; i < num_bytes; i++) {
		if (data[i] == COMMENT_START) {
			while (i < num_bytes && data[i] != '\n') data[i++] = ' ';
		}
	}

	for (long i = 0; i < num_bytes; i++) {
		const char curr_char = data[i];
		if (curr_char == SECTION_TAG_START) {
			const SectionTag* const tag = get_section_tag(i, file_contents);
			progress_char_index_to_tag_argument(&i, tag, file_contents);
			parse_section(&i, file_contents, tag, eds);
		}
		else if (!isspace(curr_char))
			FAIL(ParseLevelFile, "Lone character '%c' for level '%s'.", curr_char, file_contents -> file_name);
	}
}

void init_editor_state_from_ddl_file(EditorState* const eds, const char* const filename) {
	FileContents file_contents = read_file_contents(filename);
	parse_ddl_file(eds, &file_contents);
	free(file_contents.data);
}
