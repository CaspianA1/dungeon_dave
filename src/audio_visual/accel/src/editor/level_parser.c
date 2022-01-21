#include "level_parser.h"

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

// Sets num_textures, map_size, heightmap, texture_id_map, map_name, renderer, and textures
void parse_json_file(EditorState* const eds, FileContents* const file_contents) {
	puts("About to parse json file");
	(void) eds;
	(void) file_contents;
}

void init_editor_state_from_json_file(EditorState* const eds, const char* const file_name) {
	FileContents file_contents = read_file_contents(file_name);
	parse_json_file(eds, &file_contents);
	free(file_contents.data);
}

