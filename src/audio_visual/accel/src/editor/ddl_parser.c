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
	return (FileContents) {data, file_name, num_bytes};
}

static void parse_section_tag(const long* const char_index, const FileContents file_contents) {
	/* While not reached space or newline, collect tag name
	Curr tag types: name, heightmap, texture_id_map, wall_textures 
	Also, a variant of one-line tags
	Goal is to get correct section tag out, by comparing the string up until a newline (do the multiline variant first) */

	// No need to worry about overflow here since strncmp accounts for a possible null terminator
	const char* const tag_start = file_contents.data + *char_index + 1;
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
		FAIL(ParseLevelFile, "did not recognize a level tag for '%s'.", file_contents.file_name);

	else printf("Found a tag match: '%s'\n", matching_tag -> name);
}

static void parse_ddl_file(EditorState* const editor_state, const FileContents file_contents) {
	for (long i = 0; i < file_contents.num_bytes; i++) {

		const char curr_char = file_contents.data[i];
		switch (curr_char) {
			case COMMENT_START: // This skips until a newline is reached
				while (i < file_contents.num_bytes && file_contents.data[i] != '\n') i++;
				break;
			case SECTION_TAG_START:
				puts("Start of section");
				parse_section_tag(&i, file_contents);
				break;
		}
	}
}

void init_editor_state_from_ddl_file(EditorState* const editor_state, const char* const filename) {
	const FileContents file_contents = read_file_contents(filename);
	parse_ddl_file(editor_state, file_contents);
	free(file_contents.data);
}
