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

// This discards whitespace after an index until a non-whitespace character is reached
static void discard_whitespace_after_index(long* const char_index_ref, const FileContents* const file_contents) {
	long char_index = *char_index_ref + 1;
	while ((char_index < file_contents -> num_bytes) && isspace(file_contents -> data[char_index])) char_index++;
	*char_index_ref = char_index;
}

static const SectionTag* get_section_tag(const long char_index, const FileContents* const file_contents) {
	/* While not reached space or newline, collect tag name
	Curr tag types: name, heightmap, texture_id_map, wall_textures 
	Also, a variant of one-line tags
	Goal is to get correct section tag out, by comparing the string up until a newline (do the multiline variant first) */

	// No need to worry about overflow here since strncmp accounts for a possible null terminator
	const char* const tag_start = file_contents -> data + char_index + 1;
	const SectionTag* matching_tag = NULL;

	for (byte i = 0; i < num_section_tag_types; i++) {
		const SectionTag* const tag_ref = section_tags + i;
		const char* const tag_name = tag_ref -> name;

		if (strncmp(tag_name, tag_start, strlen(tag_name)) == 0) {
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

	if (tag -> is_one_line) {
		// Only expecting name for now; alloc map name for editor state, until any whitespace reached
	}
	else {

	}
}

static void parse_ddl_file(EditorState* const editor_state, const FileContents* const file_contents) {
	const char* const data = file_contents -> data;
	const long num_bytes = file_contents -> num_bytes;

	for (long i = 0; i < num_bytes; i++) {

		const char curr_char = data[i];
		switch (curr_char) {
			case COMMENT_START: // This skips until a newline is reached
				while (i < num_bytes && data[i] != '\n') i++;
				break;
			case SECTION_TAG_START: {
				const SectionTag* const section_tag = get_section_tag(i, file_contents);
				i += strlen(section_tag -> name); // + 1l b/c starting at the character right after the tag
				discard_whitespace_after_index(&i, file_contents);
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
