#ifndef DDL_PARSER_H
#define DDL_PARSER_H

// DDL = Dungeon Dave level. Later, keep file handle to write to it later.
#include "editor.h"

#define COMMENT_START '-'
#define SECTION_TAG_START '@'

//////////

typedef struct {
	const char* const file_name;
	char* const data;
	const long num_bytes;
} FileContents;

//////////

#define SECTION_PARSER_NAME(type) parse_##type##_section

#define SECTION_PARSER_SIGNATURE (const long start_char_index, const long end_char_index,\
	const FileContents* const file_contents, const EditorState* const eds)

#define SECTION_PARSER_DEF(type) static void SECTION_PARSER_NAME(type) SECTION_PARSER_SIGNATURE

typedef struct { // TODO: link up each section tag with a parser fn ptr
	const char* const name;
	void (*const section_parser) SECTION_PARSER_SIGNATURE;
} SectionTag;

SECTION_PARSER_DEF(name);
SECTION_PARSER_DEF(heightmap);
SECTION_PARSER_DEF(texture_id_map);
SECTION_PARSER_DEF(wall_texture);

enum {num_section_tag_types = 4};

static const SectionTag section_tags[num_section_tag_types] = {
	{"name", SECTION_PARSER_NAME(name)},
	{"heightmap", SECTION_PARSER_NAME(heightmap)},
	{"texture_id_map", SECTION_PARSER_NAME(texture_id_map)},
	{"wall_textures", SECTION_PARSER_NAME(wall_texture)}
};

//////////

// Excluded: section parsers, read_file_contents, progress_char_index_to_tag_argument, get_section_tag, parse_section

void init_editor_state_from_ddl_file(EditorState* const eds, const char* const filename);

#endif
