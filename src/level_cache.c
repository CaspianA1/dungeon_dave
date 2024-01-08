#include "level_cache.h"
#include "utils/safe_io.h" // For `ASSET_PATH_PREFIX`, and `get_temp_asset_path`
#include "utils/failure.h" // For `FAIL`
#include <sys/stat.h> // TODO: support Windows for this too
#include <time.h>

/* TODO:
- Do I need to worry about endian-ness here? I should test this on a big-endian machine.
- Can I compress the cache, to make it take up less space?

- Later on, genericize/generalize each entry in the cache by these properties:
	- Source file to check for modification
	- Fn pointer that returns data to caller
	- Fn pointer that provides disk-writable data to write

- Should that go in its own file, `file_cache.c`? (in `utils`, probably)

- Note: for keeping more cache entries, structure the file like this:
	1. Each entry starts with the change date for a file
	2. That then follows by cache data

- That then keeps going until the end of the file

- Someone could definitely tamper with this, but eventually I'll try
	to find a way to sign the cache files, or compare them against
	a checksum, or something like that

- Maybe use run-length encoding, or LZ77
*/

// The returned string should be freed with `dealloc`
static char* get_cache_path_from_level_path(const char* const level_path) {
	const char* const after_last_slash_pos = strrchr(level_path, '/') + 1;
	const char* const last_period_pos = strrchr(after_last_slash_pos, '.');
	const int isolated_level_name_length = last_period_pos - after_last_slash_pos;

	return make_formatted_string("%scache/%.*s.cache", ASSET_PATH_PREFIX, isolated_level_name_length, after_last_slash_pos);
}

static time_t get_last_file_modification_time(const char* const path) {
	struct stat level_file_attributes;
	stat(path, &level_file_attributes); // TODO: check this for failure
	return level_file_attributes.st_mtime;
}

// TODO: upon this failure, remove the cache file too
static void fail_for_cache_operation(const char* const cache_path, const char* const aspect_that_failed) {
	FAIL(WorkWithLevelCache, "Couldn't %s the level cache file '%s'", aspect_that_failed, cache_path);
}

//////////

static LevelCache create_cache(
	FILE* const cache_file,
	const char* const cache_path,
	const LevelCacheConfig* const config,
	const time_t last_modification_time) {

	fwrite(&last_modification_time, sizeof(last_modification_time), 1, cache_file);

	const Heightmap heightmap = config -> ambient_occlusion.heightmap;
	const map_pos_component_t max_y = config -> ambient_occlusion.max_y;

	ao_value_t* cpu_ao_map_data;

	const AmbientOcclusionMap ao_map = init_ao_map_with_copy_on_cpu(
		heightmap, max_y, config -> ambient_occlusion.compute_config,
		&cpu_ao_map_data
	);

	const size_t amt_to_write = heightmap.size.x * heightmap.size.z * max_y;
	const size_t amt_written = fwrite(cpu_ao_map_data, sizeof(ao_value_t), amt_to_write, cache_file);

	if (amt_to_write != amt_written)
		fail_for_cache_operation(cache_path, "write ambient occlusion data to");

	dealloc(cpu_ao_map_data);

	return (LevelCache) {.ao_map = ao_map};
}

static LevelCache parse_cache(FILE* const cache_file,
	const LevelCacheConfig* const config,
	const char* const cache_path) {

	// Next, moving the file pointer ahead from the last modification time
	if (fseek(cache_file, sizeof(time_t), SEEK_SET) == -1)
		fail_for_cache_operation(cache_path, "parse the source file modification date for");

	const Heightmap heightmap = config -> ambient_occlusion.heightmap;
	const map_pos_component_t max_y = config -> ambient_occlusion.max_y;
	const size_t expected_num_entries = heightmap.size.x * heightmap.size.z * max_y;

	ao_value_t* const ao_data = alloc(expected_num_entries, sizeof(ao_value_t));
	const size_t amt_read = fread(ao_data, sizeof(ao_value_t), expected_num_entries, cache_file);
	if (amt_read != expected_num_entries) fail_for_cache_operation(cache_path, "read the ambient occlusion data for");

	const AmbientOcclusionMap ao_map = init_ao_map_from_cpu_copy(heightmap, max_y, ao_data);
	dealloc(ao_data);
	return (LevelCache) {.ao_map = ao_map};
}

LevelCache get_level_cache(const char* const level_path_unprefixed, const LevelCacheConfig* const config) {
	// TODO: can I move a lot of these file operations into `safe_io.h`?

	const char* const level_path = get_temp_asset_path(level_path_unprefixed);
	const time_t last_modification_time = get_last_file_modification_time(level_path);
	char* const cache_path = get_cache_path_from_level_path(level_path);
	FILE* cache_file = fopen(cache_path, "rb");

	LevelCache cache;

	if (cache_file == NULL) {
		// printf("Creating a cache, because the cache '%s' does not exist yet\n", cache_path);
		cache_file = fopen(cache_path, "wb"); // Only need writing at this point
		cache = create_cache(cache_file, cache_path, config, last_modification_time);
	}
	else {
		time_t cached_last_modification_time;
		const size_t modification_time_size = sizeof(cached_last_modification_time);

		const size_t amt_read = fread(&cached_last_modification_time, modification_time_size, 1, cache_file);
		if (amt_read != 1) fail_for_cache_operation(cache_path, "parse the last modification time for");

		if (last_modification_time == cached_last_modification_time) {
			// puts("Cache is in-sync");
			cache = parse_cache(cache_file, config, cache_path);
		}
		else {
			// printf("Cache must be updated. Last = %zu, cached = %zu.\n", last_modification_time, cached_last_modification_time);

			// Clearing the cache by opening it with the `w` mode
			cache_file = freopen(cache_path, "wb", cache_file);
			cache = create_cache(cache_file, cache_path, config, last_modification_time);
		}
	}

	dealloc(cache_path);
	fclose(cache_file);

	return cache;
}
