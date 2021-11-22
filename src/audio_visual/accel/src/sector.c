#define inlinable static inline
#define wmalloc malloc
#define wfree free

#define bit_is_set(bits, mask) ((bits) & (mask))
#define set_bit(bits, mask) ((bits) |= (mask))

typedef struct {
	int chunk_dimensions[2];
	size_t alloc_bytes;
	byte* data;
} StateMap;

#include "../../../main/statemap.c"

//////////

const byte init_sector_alloc_size = 20;
const float sector_realloc_rate = 1.5f;

typedef struct {
	const byte height, origin[2];
	byte size[2];
} Sector;

typedef struct {
	Sector* sectors;
	int length, max_alloc;

	GLuint vbo;
	GLsizei num_vertices;
} SectorList;

//////////

SectorList init_sector_list(const int init_size) {
	return (SectorList) {
		.sectors = malloc(init_size * sizeof(Sector)),
		.length = 0,
		.max_alloc = init_size,
		.num_vertices = 0
	};
}

void push_to_sector_list(SectorList* const s, const Sector* const sector) {
	if (s -> length == s -> max_alloc)
		s -> sectors = realloc(s -> sectors, (s -> max_alloc *= sector_realloc_rate) * sizeof(Sector));

	memcpy(s -> sectors + s -> length++, sector, sizeof(Sector));
}

void print_sector_list(const SectorList* const s) {
	puts("[");
	for (int i = 0; i < s -> length; i++) {
		const Sector* const sector = &s -> sectors[i];
		printf("\t{.height = %d, .origin = {%d, %d}, .size = {%d, %d}}\n",
			sector -> height, sector -> origin[0], sector -> origin[1],
			sector -> size[0], sector -> size[1]);
	}
	puts("]");
}

void deinit_sector_list(const SectorList* const sector_list) {
	glDeleteBuffers(1, &sector_list -> vbo);
	free(sector_list -> sectors);
}

//////////

byte* map_point(byte* const map, const byte x, const byte y, const byte map_width) {
	return map + (y * map_width + x);
}

// Gets length acros, and then adds to area size y until out of map or length across not eq
Sector form_sector_area(Sector sector, const StateMap traversed_points,
	const byte* const map, const byte map_width, const byte map_height) {

	byte top_right_corner = sector.origin[0];

	while (top_right_corner < map_width && *map_point((byte*) map, top_right_corner, sector.origin[1], map_width) == sector.height) {
		sector.size[0]++;
		top_right_corner++;
	}

	// Now, area.size[0] equals the first horizontal length of equivalent height found
	for (byte y = sector.origin[1]; y < map_height; y++, sector.size[1]++) {
		for (byte x = sector.origin[0]; x < top_right_corner; x++) {
			// If consecutive heights didn't continue
			if (*map_point((byte*) map, x, y, map_width) != sector.height)
				goto clear_map_area;
		}
	}

	clear_map_area:

	for (byte y = sector.origin[1]; y < sector.origin[1] + sector.size[1]; y++) {
		for (byte x = sector.origin[0]; x < sector.origin[0] + sector.size[0]; x++)
			set_statemap_bit(traversed_points, x, y);
	}

	return sector;
}

SectorList generate_sectors_from_heightmap(const byte* const heightmap, const byte map_width, const byte map_height) {
	SectorList sector_list = init_sector_list(init_sector_alloc_size);

	/* StateMap used instead of copy of heightmap with null map points, b/c 1. less bytes used
	and 2. for forming faces, will need original heightmap to be unmodified */
	const StateMap traversed_points = init_statemap(map_width, map_height);

	for (byte y = 0; y < map_height; y++) {
		for (byte x = 0; x < map_width; x++) {
			if (get_statemap_bit(traversed_points, x, y)) continue;

			const byte height = *map_point((byte*) heightmap, x, y, map_width);
			const Sector seed_area = {.height = height, .origin = {x, y}, .size = {0, 0}};
			const Sector expanded_area = form_sector_area(seed_area, traversed_points, heightmap, map_width, map_height);
			push_to_sector_list(&sector_list, &expanded_area);
		}
	}

	deinit_statemap(traversed_points);

	return sector_list;
}

void init_sector_list_vbo(SectorList* const sector_list) {
	const int num_sectors = sector_list -> length;
	size_t total_bytes = 0, total_components = 0;

	for (int i = 0; i < num_sectors; i++)
		total_bytes += (sector_list -> sectors[i].height == 0)
			? bytes_per_face : bytes_per_mesh;

	mesh_type_t* const vertices = malloc(total_bytes);

	for (int i = 0; i < num_sectors; i++) {
		const Sector sector = sector_list -> sectors[i];
		const mesh_type_t origin[3] = {sector.origin[0], sector.height, sector.origin[1]};

		if (sector.height == 0) { // Flat sector
			create_height_zero_mesh(origin, sector.size, vertices + total_components);
			sector_list -> num_vertices += vertices_per_triangle * triangles_per_face;
			total_components += vars_per_face;
		}
		else {
			const mesh_type_t size[3] = {sector.size[0], sector.height, sector.size[1]};
			create_sector_mesh(origin, size, vertices + total_components);
			sector_list -> num_vertices += vertices_per_triangle * triangles_per_mesh;
			total_components += vars_per_mesh;
		}
	}

	glGenBuffers(1, &sector_list -> vbo);
	glBindBuffer(GL_ARRAY_BUFFER, sector_list -> vbo);
	glBufferData(GL_ARRAY_BUFFER, total_bytes, vertices, GL_STATIC_DRAW);
}
