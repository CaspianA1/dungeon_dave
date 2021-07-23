#ifdef TRACK_MEMORY

unsigned
	alloc_count = 0,
	malloc_count = 0,
	calloc_count = 0,
	realloc_count = 0,
	free_count = 0;

inlinable void* wmalloc(size_t size) { // w = wrap
	malloc_count++;
	alloc_count++;
	void* ptr = malloc(size);
	// printf("%p alloc, %d alloc total\n", ptr, alloc_count);
	return ptr;
}

inlinable void* wcalloc(size_t nitems, size_t size) {
	calloc_count++;
	alloc_count++;
	return calloc(nitems, size);
}

inlinable void* wrealloc(void* ptr, size_t new_size) {
	realloc_count++;
	return realloc(ptr, new_size);
}

inlinable void wfree(void* ptr) {
	if (ptr == NULL) {
		printf("Error: attempt to free a null pointer\n");
		abort();
	}
	free_count++;
	// printf("%p free, %d free total\n", ptr, free_count);
	free(ptr);
	ptr = NULL;
}

void dynamic_memory_report(void) {
	printf("Leak report:\n"
		"There were %u allocations made.\n"
		"There were %u calls to malloc, %u calls to calloc, and %u calls to realloc.\n"
		"There were %u deallocations made.\n",
		alloc_count, malloc_count, calloc_count, realloc_count, free_count);

	if (alloc_count > free_count)
		printf("You have a memory leak! (%u weren't freed.)\n", alloc_count - free_count);
	else if (free_count > alloc_count)
		printf("You freed too much memory! (%u pointers were accidentally freed.)\n",
			free_count - alloc_count);
	else
		printf("You have no memory problems.\n");
}

#else

#define wmalloc malloc
#define wcalloc calloc
#define wrealloc realloc
#define wfree free

#endif

/////

inlinable double to_radians(const double degrees) {
	return degrees * M_PI / 180.0;
}

inlinable byte doubles_eq(const double a, const double b, const double epsilon) {
	return fabs(a - b) < epsilon;
}

inlinable void tick_delay(const Uint32 before) {
	const int wait = settings.max_delay - (SDL_GetTicks() - before);
	if (wait > 0) SDL_Delay(wait);
}

inlinable byte map_point(const byte* const map, const int x, const int y) {
	return map[y * current_level.map_size.x + x];
}

inlinable byte point_exists_at(const double x, const double y, const double z) {
	const byte point_val = map_point(current_level.wall_data, x, y);
	return point_val && z < current_level.get_point_height(point_val, (vec) {x, y});
}

inlinable void set_map_point(byte* const map, const byte val, const int x, const int y, const int map_width) {
	map[y * map_width + x] = val;
}

inlinable void align_from_out_of_vert_bounds(int* const val) {
	if (*val < 0) *val = 0;
	else if (*val >= settings.screen_height) *val = settings.screen_height - 1;
}

void update_val_buffers(const int screen_x, int wall_top, int wall_bottom, const float dist,
	const float cos_beta, const vec dir) {

	BufferVal* buffer_val = &val_buffer[screen_x];
	for (int x = screen_x; x < screen_x + settings.ray_column_width; x++, buffer_val++) {
		buffer_val -> depth = dist;
		buffer_val -> cos_beta = cos_beta;
		buffer_val -> dir = dir;
	}

	align_from_out_of_vert_bounds(&wall_top);
	align_from_out_of_vert_bounds(&wall_bottom);

	void set_statemap_bit(const StateMap, const int, const int);
	for (int y = wall_top; y < wall_bottom; y++)
		set_statemap_bit(occluded_by_walls, screen_x, y);
}

/////

#define vec_fill _mm_set1_pd

inlinable byte vec_delta_exceeds(const vec a, const vec b, const double dist) {
	const vec delta = a - b;
	const double dist_squared = delta[0] * delta[0] + delta[1] * delta[1];
	return dist_squared > dist * dist;
}

inlinable vec vec_diff(const vec a, const vec b) {
	const vec delta = a - b;
	return (vec) {fabs(delta[0]), fabs(delta[1])};
}

inlinable byte vec_in_range(const double p, const vec range) {
	return p >= range[0] - small_double_epsilon && p <= range[1] + small_double_epsilon;
}

inlinable vec vec_line_pos(const vec pos, const vec dir, const double slope) {
	return dir * vec_fill(slope) + pos;
}

inlinable ivec ivec_from_vec(const vec v) {
	return (ivec) {(int) floor(v[0]), (int) floor(v[1])};
}

inlinable byte ivec_out_of_bounds(const ivec v) {
	return v.x < 0 || v.x > current_level.map_size.x - 1 || v.y < 0 || v.y > current_level.map_size.y - 1;
}
