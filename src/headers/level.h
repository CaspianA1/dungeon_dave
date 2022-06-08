#ifndef LEVEL_H
#define LEVEL_H

#include "buffer_defs.h"
#include "list.h"
#include "csm.h"

typedef struct {
	const struct {
		const GLchar *const skybox, *background_theme;
		const List wall_texture;
	} asset_paths;

	const List wall_texture_paths;

	//////////

	const vec3 init_pos;
	const CascadedShadowSpec shadow_spec;

	/*
	- Light params
	- How to handle switching weapons? Perhaps just one weapon per level for now
	- Rescaling of assets?
	*/

	const byte map_width, map_height;
	const byte* const heightmap;
} LevelDescription;

void level_test(void);

#endif
