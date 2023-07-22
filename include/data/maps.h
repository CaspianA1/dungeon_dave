#ifndef MAPS_H
#define MAPS_H

#include "utils/typedefs.h" // For `map_pos_component_t`

enum { // These are enums so that they can define compile-time constant arrays
	level_one_width = 25, level_one_height = 15,
	architecture_width = 50, architecture_height = 50,
	fortress_width = 50, fortress_height = 50,
	palace_width = 40, palace_height = 40,
	tpt_width = 10, tpt_height = 20,
	pyramid_width = 30, pyramid_height = 40,
	maze_width = 90, maze_height = 61,
	test_width = 8, test_height = 5,
	terrain_width = 255, terrain_height = 255,
	terrain_2_width = 255, terrain_2_height = 213,
	tiny_width = 7, tiny_height = 7,
	checker_width = 255, checker_height = 255,
	blank_width = 1, blank_height = 2
};

#define MAP_DEF(prefix) prefix##_heightmap[prefix##_height][prefix##_width], prefix##_texture_id_map[prefix##_height][prefix##_width]

extern const map_pos_component_t
	MAP_DEF(level_one), MAP_DEF(architecture), MAP_DEF(fortress),
	MAP_DEF(palace), MAP_DEF(tpt), MAP_DEF(pyramid), MAP_DEF(maze),
	MAP_DEF(test), MAP_DEF(terrain), MAP_DEF(terrain_2),
	MAP_DEF(tiny), MAP_DEF(checker), MAP_DEF(blank);

#undef MAP_DEF

#endif
