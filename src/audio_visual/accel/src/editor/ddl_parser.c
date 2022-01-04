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
	const SectionTag* const section_tag, const FileContents* const file_contents) {

	long char_index = *char_index_ref + strlen(section_tag -> name) + 1;
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

	else printf("Found a tag match: '%s'\n", matching_tag -> name);
	return matching_tag;
}

static void parse_section(const SectionTag* const tag, const EditorState* const editor_state, const FileContents* const file_contents) {
	(void) tag;
	(void) editor_state;
	(void) file_contents;
}

static void parse_ddl_file(EditorState* const editor_state, const FileContents* const file_contents) {
	const char* const data = file_contents -> data;
	const long num_bytes = file_contents -> num_bytes;

	for (long i = 0; i < num_bytes; i++) {

		const char curr_char = data[i];
		/* Right now, arbitrary characters can appear as non-arguments;
		fail if a character doesn't match the first 2 cases */

		switch (curr_char) {
			case COMMENT_START: // This skips until a newline is reached
				while (i < num_bytes && data[i] != '\n') i++;
				break;
			case SECTION_TAG_START: {
				const SectionTag* const section_tag = get_section_tag(i, file_contents);
				progress_char_index_to_tag_argument(&i, section_tag, file_contents);
				parse_section(section_tag, editor_state, file_contents);
				break;
			}
		}
	}
}

void init_editor_state_from_ddl_file(EditorState* const editor_state, const char* const filename) {
	const FileContents file_contents = read_file_contents(filename);
	parse_ddl_file(editor_state, &file_contents);
	free(file_contents.data);
}
