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
} FileContents;

//////////

#define SECTION_PARSER_NAME(type) parse_##type##_section
#define SECTION_PARSER_ARGS (const char* curr_token, const char* const delims, const EditorState* const eds)
#define SECTION_PARSER_RETURN_TYPE const char* // Each section parser returns the next token for the main loop to parse
#define SECTION_PARSER_DEF(type) static SECTION_PARSER_RETURN_TYPE SECTION_PARSER_NAME(type) SECTION_PARSER_ARGS

#define INITIAL_PARSER_TOKEN strtok
#define NEXT_PARSER_TOKEN() strtok(NULL, delims)
#define VARARG_SECTION_PARSER_HAS_TOKEN() ((curr_token != NULL) && (curr_token[0] != SECTION_TAG_START))

typedef struct {
	const char* const name;
	SECTION_PARSER_RETURN_TYPE (*const section_parser) SECTION_PARSER_ARGS;
} SectionTag;

SECTION_PARSER_DEF(name);
SECTION_PARSER_DEF(map_size);
SECTION_PARSER_DEF(heightmap);
SECTION_PARSER_DEF(texture_id_map);
SECTION_PARSER_DEF(wall_texture);

enum {num_section_tag_types = 5};

static const SectionTag section_tags[num_section_tag_types] = {
	{"name", SECTION_PARSER_NAME(name)},
	{"map_size", SECTION_PARSER_NAME(map_size)},
	{"heightmap", SECTION_PARSER_NAME(heightmap)},
	{"texture_id_map", SECTION_PARSER_NAME(texture_id_map)},
	{"wall_textures", SECTION_PARSER_NAME(wall_texture)}
};

////////// Excluded: read_file_contents, section parsers, remove_comments_from_file_data, parse_ddl_file

void init_editor_state_from_ddl_file(EditorState* const eds, const char* const file_name);

#endif
