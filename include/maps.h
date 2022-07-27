#ifndef MAPS_H
#define MAPS_H

#include "buffer_defs.h"

enum {
	level_one_width = 25, level_one_height = 15,
	architecture_width = 50, architecture_height = 50,
	fortress_width = 50, fortress_height = 50,
	palace_width = 40, palace_height = 40,
	tpt_width = 10, tpt_height = 20,
	pyramid_width = 30, pyramid_height = 40,
	maze_width = 90, maze_height = 61,
	test_width = 8, test_height = 5,
	terrain_width = 255, terrain_height = 255,
	tiny_width = 7, tiny_height = 7,
	checker_width = 255, checker_height = 255
};

// TODO: make a `MAP_DEF` macro

extern const byte
	level_one_heightmap[level_one_height][level_one_width],
	level_one_texture_id_map[level_one_height][level_one_width],
	architecture_heightmap[architecture_height][architecture_width],
	architecture_texture_id_map[architecture_height][architecture_width],
	fortress_heightmap[fortress_height][fortress_width],
	fortress_texture_id_map[fortress_height][fortress_width],
	palace_heightmap[palace_height][palace_width],
	palace_texture_id_map[palace_height][palace_width],
	tpt_heightmap[tpt_height][tpt_width],
	tpt_texture_id_map[tpt_height][tpt_width],
	pyramid_heightmap[pyramid_height][pyramid_width],
	pyramid_texture_id_map[pyramid_height][pyramid_width],
	maze_heightmap[maze_height][maze_width],
	maze_texture_id_map[maze_height][maze_width],
	test_heightmap[test_height][test_width],
	test_texture_id_map[test_height][test_width],
	terrain_heightmap[terrain_height][terrain_width],
	terrain_texture_id_map[terrain_height][terrain_width],
	tiny_heightmap[tiny_height][tiny_width],
	tiny_texture_id_map[tiny_height][tiny_width],
	checker_heightmap[checker_height][checker_width],
	checker_texture_id_map[checker_height][checker_width];

#endif
