#ifndef DDL_PARSER_H
#define DDL_PARSER_H

#include "editor.h"

#define COMMENT_START '-'
#define SECTION_TAG_START '@'

// DDL = Dungeon Dave level. Later, keep file handle to write to it later.
typedef struct {
	const char* const file_name;
	char* const data;
	const long num_bytes;
} FileContents;

typedef struct { // TODO: link up each section tag with a parser fn ptr
	const char* const name;
} SectionTag;

//////////

enum {num_section_tag_types = 4};

static const SectionTag section_tags[num_section_tag_types] = {
	{"name"},
	{"heightmap"},
	{"texture_id_map"},
	{"wall_textures"}
};

////////// Excluded: read_file_contents, progress_char_index_to_tag_argument, get_section_tag, parse_section

void init_editor_state_from_ddl_file(EditorState* const editor_state, const char* const filename);

#endif
