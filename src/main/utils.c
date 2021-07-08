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

inlinable byte map_point(const byte* const map, const double x, const double y) {
	return map[(int) (floor(y) * current_level.map_width + floor(x))];
}

inlinable byte point_exists_at(const double x, const double y, const double z) {
	const byte point_val = map_point(current_level.wall_data, x, y);
	return point_val && z < current_level.get_point_height(point_val, (VectorF) {x, y});
}

inlinable void set_map_point(byte* const map, const byte val, const int x, const int y, const int map_width) {
	map[y * map_width + x] = val;
}

/////

inlinable void update_val_buffers(const int screen_x, const double dist, const double cos_beta,
	const float wall_bottom, const VectorF dir) {

	for (int x = screen_x; x < screen_x + settings.ray_column_width; x++) {
		screen.z_buffer[x] = dist;
		screen.cos_beta_buffer[x] = cos_beta;
		screen.wall_bottom_buffer[x] = wall_bottom;
		screen.dir_buffer[x] = dir;
	}
}

/////

inlinable byte VectorI_out_of_bounds(const VectorI vi) {
	return vi.x < 0 || vi.x > current_level.map_width - 1 || vi.y < 0 || vi.y > current_level.map_height - 1;
}

inlinable byte VectorII_eq(const VectorI v1, const VectorI v2) {
	return v1.x == v2.x && v1.y == v2.y;
}

///// https://docs.microsoft.com/en-us/cpp/intrinsics/x86-intrinsics-list?view=msvc-160

inlinable VectorI VectorF_floor(const VectorF vf) {
	return (VectorI) {(int) floor(vf[0]), (int) floor(vf[1])};
}

#define VectorF_memset _mm_set1_pd
#define VectorFF_add _mm_add_pd
#define VectorFF_sub _mm_sub_pd
#define VectorFF_mul _mm_mul_pd
#define VectorFF_div _mm_div_pd


inlinable byte VectorFF_exceed_dist(const VectorF a, const VectorF b, const double dist) {
	const VectorF delta = VectorFF_sub(a, b);
	const double dist_squared = delta[0] * delta[0] + delta[1] * delta[1];
	return dist_squared > dist * dist;
}

inlinable VectorF VectorFF_diff(const VectorF a, const VectorF b) {
	const VectorF signed_diff = VectorFF_sub(a, b);
	return (VectorF) {fabs(signed_diff[0]), fabs(signed_diff[1])};
}

inlinable byte VectorF_in_range(const double p, const VectorF range) {
	return p >= range[0] - small_double_epsilon && p <= range[1] + small_double_epsilon;
}
inlinable VectorF VectorF_line_pos(const VectorF pos, const VectorF dir, const double slope) {
	const VectorF slope_as_vec = VectorF_memset(slope);
	return VectorFF_add(VectorFF_mul(dir, slope_as_vec), pos);
}
