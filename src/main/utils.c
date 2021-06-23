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

inlinable byte map_point(const byte* map, const double x, const double y) {
	// return current_level.wall_data[(int) floor(y)][(int) floor(x)];	
	// return current_level.wall_data[(int) (floor(y) * current_level.map_height + floor(x))];
	return map[(int) (floor(y) * current_level.map_height + floor(x))];
}

inlinable byte point_exists_at(const double x, const double y, const double z) {
	const byte point_val = map_point(current_level.wall_data, x, y);
	return point_val && z < current_level.get_point_height(point_val, (VectorF) {x, y});
}

/////

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

#define VectorF2_memset _mm256_set1_pd
#define VectorFF2_add _mm256_add_pd
#define VectorFF2_mul _mm256_mul_pd

inlinable VectorF VectorFF_diff(const VectorF a, const VectorF b) {
	return (VectorF) {fabs(a[0] - b[0]), fabs(a[1] - b[1])};
}

inlinable byte VectorF_in_range(const double p, const VectorF range) {
	return p >= range[0] - small_double_epsilon && p <= range[1] + small_double_epsilon;
}

inlinable VectorF VectorF_line_pos(const VectorF pos,
	const VectorF dir, const double slope) {

	const VectorF slope_as_vec = VectorF_memset(slope);
	return VectorFF_add(VectorFF_mul(dir, slope_as_vec), pos);
}

// http://ftp.neutrino.es/x86InstructionSet/VINSERTF128.html
inlinable VectorF2 VectorF2_line_pos(const VectorF pos,
	const VectorF dir, const VectorF slopes) {

	const VectorF2
		slopes_as_vec2 = {slopes[0], slopes[0], slopes[1], slopes[1]},
		dir_as_vec2 = {dir[0], dir[1], dir[0], dir[1]},
		pos_as_vec2 = {pos[0], pos[1], pos[0], pos[1]};

	return VectorFF2_add(VectorFF2_mul(dir_as_vec2, slopes_as_vec2), pos_as_vec2);
}

/*
vector operations needed:
	subtraction of int-based vectors (not together): {a[0] - b[0], a[1] - b[1]}
	distance: sqrt(a[0] * a[0] + b[1] * b[1])
	
	texture offset: {a[0] - floor(a[0]) * m, a[1] - floor(a[1]) * m}
	unit step size: {fabs(1.0 / a[0]), fabs(1.0 / a[1])}
	truncation: {(int) floor(a[0]), (int) floor(a[1])}
	delta between floating-point and int-based vectors: {fabs(v[0] - i[0]), fabs(v[1] - i[1])}

	handy functions: https://chryswoods.com/vector_c++/emmintrin.html

	also, make VectorI to an intrinsic at some point (need the right type first)

done:
	subtraction of floating-point vectors
	ray equation: {a[0] * d + p[0], a[1] * d + p[1]}
*/
