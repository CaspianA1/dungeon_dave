#ifdef TRACK_MEMORY

static unsigned
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
		puts("Error: attempt to free a null pointer!");
		abort();
	}
	free_count++;
	// printf("%p free, %d free total\n", ptr, free_count);
	free(ptr);
}

void dynamic_memory_report(void) {
	printf("Leak report:\nThere were %u allocations made.\nThere were %u "
		"calls to malloc, %u calls to calloc, and %u calls to realloc.\n"
		"There were %u deallocations made.\n", alloc_count, malloc_count,
		calloc_count, realloc_count, free_count);

	if (alloc_count > free_count)
		printf("You have a memory leak! (%u weren't freed.)\n", alloc_count - free_count);
	else if (free_count > alloc_count)
		printf("You freed too much memory! (%u pointers were accidentally freed.)\n",
			free_count - alloc_count);
	else
		puts("You have no memory problems.");
}

#else

#define wmalloc malloc
#define wcalloc calloc
#define wrealloc realloc
#define wfree free

#endif

//////////

#define bit_is_set(bits, mask) ((bits) & (mask))
#define set_bit(bits, mask) ((bits) |= (mask))
#define clear_bit(bits, mask) ((bits) &= ~(mask))
#define bit_to_x(bits, mask, x) ((bits) ^= (-(x) ^ (bits)) & (mask))

//////////

inlinable double to_radians(const double degrees) {
	return degrees * M_PI / 180.0;
}

inlinable byte doubles_eq(const double a, const double b) {
	return fabs(a - b) < almost_zero;
}

inlinable void tick_delay(const Uint32 before) {
	const int wait = settings.max_delay - (SDL_GetTicks() - before);
	if (wait > 0) SDL_Delay(wait);
}

inlinable byte* map_point(byte* const map, const int x, const int y) {
	return map + (y * current_level.map_size.x + x);
}

byte point_exists_at(const double x, const double y, const double height) {
	const byte out_of_bounds = (x < 1.0) || (x > current_level.map_size.x - 1)
		|| (y < 1.0) || (y > current_level.map_size.y - 1);

	return out_of_bounds || (height < *map_point(current_level.heightmap, x, y));
}

void update_buffers(const int screen_x, const float dist, const float cos_beta, const vec dir) {
	FloorcastBufferVal* floorcast_buffer_val = floorcast_val_buffer + screen_x;

	for (int x = screen_x; x < screen_x + settings.ray_column_width; x++, floorcast_buffer_val++) {
		floorcast_buffer_val -> one_over_cos_beta = 1.0f / cos_beta;
		floorcast_buffer_val -> dir = dir;
		depth_buffer[x] = dist;
	}
}

inlinable double get_projected_y(const double horizon_line, const double half_screen_h,
	const double screen_h, const double world_h) {

	return horizon_line - half_screen_h + world_h * screen_h;
}

//////////

#define vec_fill _mm_set1_pd
#define vec_fill_3D _mm_set1_ps
#define vec_trunc(vec) _mm_round_pd(vec, _MM_FROUND_TRUNC)

inlinable byte vec_delta_exceeds(const vec a, const vec b, const double max_dist) {
	const vec delta = a - b;
	const double dist_squared = delta[0] * delta[0] + delta[1] * delta[1];
	return dist_squared > max_dist * max_dist;
}

inlinable byte vec_out_of_bounds(const vec v) {
	return (v[0] < 0.0) || (v[0] > current_level.map_size.x - 1.0)
		|| (v[1] < 0.0) || (v[1] > current_level.map_size.y - 1.0);
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
	return (v.x < 0) || (v.x > current_level.map_size.x - 1)
		|| (v.y < 0) || (v.y > current_level.map_size.y - 1);
}
