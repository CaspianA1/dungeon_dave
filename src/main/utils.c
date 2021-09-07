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

//////////

#define num_leading_zeroes __builtin_clz
#define exp_for_pow_of_2(num) (__builtin_ffs(num) - 1)

inlinable double to_radians(const double degrees) {
	return degrees * M_PI / 180.0;
}

inlinable byte doubles_eq(const double a, const double b) {
	return fabs(a - b) < small_double_epsilon;
}

inlinable void nth_bit_to_x(byte* const bits, const byte n, const byte x) {
	*bits ^= (-x ^ *bits) & (1 << n);
}

inlinable void set_nth_bit(byte* const bits, const byte n) {
	*bits |= (1 << n);
}

inlinable void clear_nth_bit(byte* const bits, const byte n) {
	*bits &= ~(1 << n);
}

inlinable void tick_delay(const Uint32 before) {
	const int wait = settings.max_delay - (SDL_GetTicks() - before);
	if (wait > 0) SDL_Delay(wait);
}

inlinable double double_max(const double a, const double b) {
	return (a > b) ? a : b;
}

inlinable byte map_point(const byte* const map, const int x, const int y) {
	return map[y * current_level.map_size.x + x];
}

byte point_exists_at(const double x, const double y, const double z) {
	const vec pos_2D = {x, y};

	const byte point_val = map_point(current_level.wall_data, x, y);
	return (point_val && z < current_level.get_point_height(point_val, pos_2D)) ||
		(pos_2D[0] < 1.0 || pos_2D[0] > current_level.map_size.x - 1 || pos_2D[1] < 1.0 || pos_2D[1] > current_level.map_size.y - 1);
}

inlinable void set_map_point(byte* const map, const byte val, const int x, const int y, const int map_width) {
	map[y * map_width + x] = val;
}

void update_val_buffer(const int screen_x, const float dist, const float cos_beta, const vec dir) {
	BufferVal* buffer_val = &val_buffer[screen_x];
	for (int x = screen_x; x < screen_x + settings.ray_column_width; x++, buffer_val++) {
		buffer_val -> depth = dist;
		buffer_val -> one_over_cos_beta = 1.0f / cos_beta;
		buffer_val -> dir = dir;
	}
}

//////////

#define vec_fill _mm_set1_pd
#define vec_trunc(vec) _mm_round_pd(vec, _MM_FROUND_TRUNC)

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
	return (ivec) {(int) v[0], (int) v[1]};
}

inlinable vec vec_from_ivec(const ivec v) {
	return (vec) {v.x, v.y};
}

inlinable SDL_Rect rect_from_ivecs(const ivec v1, const ivec v2) {
	return (SDL_Rect) {v1.x, v1.y, v2.x, v2.y};
}

inlinable byte ivec_out_of_bounds(const ivec v) {
	return v.x < 0 || v.x > current_level.map_size.x - 1 || v.y < 0 || v.y > current_level.map_size.y - 1;
}

inlinable vec align_vec_from_out_of_bounds(const vec v) {
	return (vec) {fmod(fabs(v[0]), current_level.map_size.x), fmod(fabs(v[1]), current_level.map_size.y)};
}
