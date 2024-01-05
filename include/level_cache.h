#ifndef LEVEL_CACHE_H
#define LEVEL_CACHE_H

#include "rendering/ambient_occlusion.h" // For `AmbientOcclusionMap`
#include "utils/typedefs.h" // For `Heightmap`

typedef struct {
	AmbientOcclusionMap ao_map;
} LevelCache;

typedef struct {
	struct {
		const Heightmap heightmap;
		const map_pos_component_t max_y;
		const AmbientOcclusionComputeConfig* const compute_config;
	} ambient_occlusion;
} LevelCacheConfig;

/* Excluded:
get_cache_path_from_level_path, get_last_file_modification_time,
create_cache, fail_for_cache_operation, parse_cache */

/* TODO:
- Should the input be a `GLchar`?
- Possibly include a deallocation function
*/

LevelCache get_level_cache(const char* const level_path_unprefixed, const LevelCacheConfig* const config);

#endif
