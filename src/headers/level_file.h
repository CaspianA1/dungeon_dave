#ifndef LEVEL_FILE_H
#define LEVEL_FILE_H

#include "buffer_defs.h"
#include "list.h"
#include "csm.h"

typedef struct {
	const struct {
		const GLchar *const skybox, *background_theme;
		const List wall_texture;
	} asset_paths;

	//////////

	const vec3 init_pos;
	const CascadedShadowSpec shadow_spec;

	/*
	- Light params
	- Normal map params
	- How to handle switching weapons? Perhaps just one weapon per level for now
	- Rescaling of assets?
	*/

	const byte map_width, map_height;
	const byte* const heightmap;
} LevelDescription;

// Excluded: get_json_type_string, print_json, init_json_from_path, check_json_structural_equivalence

void level_file_test(void);

#endif
