#include "demo_11.c"
#include "../sector.c"

/*
- Sectors contain their meshes
- To begin with, don't clip sector heights based on adjacent heights
- Sectors are rectangular

- Not perfect, but sectors + their meshes for clipping and rendering, and texmaps + heightmaps for game logic
- Ideal: BSPs, but not worth time
- To start, one vbo + texture ptr per sector
- For map edges, only render inside + top face

_____
- Clip sectors based on adjacent heights
- For neighboring sectors with the same height, make them into flat 2D planes
- Or in the general case, if a plane is partially invisible, truncate it; otherwise, remove it
- Find which sectors are behind, and then skip rendering those

- Billboard sprites
- Read sprite crop from spritesheet

- Blit 2D sprite to whole screen
- Blit color rect to screen
*/

StateGL configurable_demo_12_init(byte* const heightmap, const byte map_width, const byte map_height) {
	StateGL sgl = {.vertex_array = init_vao()};

	const SectorList sectors = generate_sectors_from_heightmap(heightmap, map_width, map_height);

	sgl.num_vertex_buffers = sectors.length;
	sgl.vertex_buffers = malloc(sgl.num_vertex_buffers * sizeof(GLuint));
	glGenBuffers(sgl.num_vertex_buffers, sgl.vertex_buffers);

	for (int i = 0; i < sectors.length; i++) {
		Sector* const sector = sectors.data + i;
		const SectorArea area = sector -> area;
		sector -> vbo = sgl.vertex_buffers[i];
		const plane_type_t origin[3] = {area.origin[0], area.height, area.origin[1]};

		plane_type_t* mesh;
		byte mesh_bytes;

		if (area.height == 0) { // Flat sector
			mesh = create_height_zero_mesh(origin, area.size);
			mesh_bytes = bytes_per_height_zero_mesh;
		}
		else {
			const plane_type_t size[3] = {area.size[0], area.height, area.size[1]};
			mesh = create_sector_mesh(origin, size);
			mesh_bytes = bytes_per_mesh;
		}

		glBindBuffer(GL_ARRAY_BUFFER, sector -> vbo);
		glBufferData(GL_ARRAY_BUFFER, mesh_bytes, mesh, GL_STATIC_DRAW);

		free(mesh);
	}
	// any_data stores sector meshes
	SectorList* const sector_list_on_heap = malloc(sizeof(SectorList));
	*sector_list_on_heap = sectors;
	sgl.any_data = sector_list_on_heap; // any_data freed in demo_12_deinit

	sgl.shader_program = init_shader_program(demo_4_vertex_shader, demo_4_fragment_shader);
	enable_all_culling();

	return sgl;	
}

StateGL demo_12_palace_init(void) {
	enum {map_width = 40, map_height = 40};

	static const byte heightmap[map_height][map_width] = {
		{3, 3, 3, 3, 3, 3, 3, 3, 3, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5},
		{3, 0, 0, 3, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10,10,10,10,10,0, 0, 0, 5},
		{3, 0, 0, 3, 0, 0, 3, 0, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 10,0, 10,10,10,10,10,0, 0, 0, 5},
		{3, 0, 0, 0, 0, 0, 3, 0, 6, 8, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 6, 10,0, 10,10,10,10,10,0, 0, 0, 5},
		{3, 3, 3, 3, 0, 0, 3, 0, 6, 8, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 6, 10,0, 10,10,10,10,10,0, 0, 0, 5},
		{3, 0, 0, 0, 0, 0, 3, 0, 8, 8, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 6, 10,0, 10,10,10,10,10,0, 0, 0, 5},
		{5, 0, 0, 0, 0, 0, 5, 0, 8, 8, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 8, 1, 1, 1, 1, 1, 1, 6, 10,0, 10,10,10,10,10,0, 0, 0, 5},
		{5, 0, 0, 0, 0, 0, 5, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 8, 1, 1, 1, 1, 1, 1, 1, 6, 10,0, 10,10,10,10,10,0, 0, 0, 5},
		{5, 0, 0, 0, 0, 0, 5, 0, 8, 8, 1, 1, 1, 1, 1, 1, 1, 1, 1, 8, 1, 1, 1, 1, 1, 1, 1, 1, 6, 10,0, 10,10,10,10,10,0, 0, 0, 5},
		{5, 1, 1, 1, 1, 5, 5, 0, 8, 8, 1, 1, 1, 1, 1, 1, 1, 1, 8, 1, 1, 1, 1, 1, 1, 1, 1, 1, 6, 10,0, 10,10,10,10,10,0, 0, 0, 5},
		{6, 2, 2, 2, 2, 5, 5, 0, 6, 8, 2, 2, 2, 2, 2, 2, 2, 8, 2, 2, 2, 2, 8, 2, 2, 2, 2, 2, 6, 10,0, 10,10,10,10,10,1, 1, 1, 5},
		{6, 3, 3, 3, 3, 5, 5, 0, 6, 8, 3, 3, 3, 3, 3, 3, 8, 3, 3, 3, 3, 8, 2, 2, 2, 2, 2, 2, 6, 10,0, 10,10,10,10,10,1, 1, 1, 5},
		{6, 3, 3, 3, 3, 0, 0, 0, 6, 8, 8, 3, 3, 3, 3, 8, 3, 3, 3, 3, 8, 3, 2, 2, 2, 2, 2, 2, 6, 10,0, 0, 0, 0, 0, 0, 0, 0, 0, 5},
		{6, 3, 3, 3, 3, 6, 6, 6, 6, 6, 8, 8, 8, 8, 8, 3, 3, 3, 3, 8, 3, 3, 2, 2, 2, 2, 2, 2, 6, 10,0, 0, 10,0, 10,0, 10,0, 10,5},
		{6, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 8, 3, 3, 3, 2, 2, 2, 2, 2, 2, 6, 10,0, 0, 0, 0, 0, 0, 0, 0, 0, 5},
		{6, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 8, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 6, 10,0, 0, 10,0, 10,0, 10,0, 10,5},
		{6, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 8, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 6, 10,0, 0, 0, 0, 0, 0, 0, 0, 0, 5},
		{6, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 8, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 6, 10,0, 10,10,10,10,10,10,10,0, 5},
		{6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 8, 8, 8, 8, 8, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 6, 10,0, 0, 0, 0, 0, 0, 0, 0, 0, 5},
		{5, 2, 2, 2, 2, 2, 2, 2, 2, 6, 8, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 6, 10,0, 0, 0, 0, 0, 10,0, 10,10,5},
		{5, 2, 1, 1, 1, 1, 1, 1, 2, 6, 8, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 6, 10,0, 10,0, 10,10,0, 0, 0, 0, 5},
		{5, 2, 1, 0, 0, 0, 0, 1, 2, 6, 8, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 6, 10,0, 10,0, 10,0, 0, 0, 0, 0, 5},
		{5, 2, 1, 0, 0, 0, 0, 1, 2, 6, 8, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 6, 10,0, 10,0, 10,0, 0, 0, 0, 0, 5},
		{5, 2, 1, 0, 0, 0, 0, 1, 2, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 3, 3, 3, 2, 2, 2, 2, 2, 2, 6, 10,0, 0, 0, 0, 0, 0, 0, 0, 0, 5},
		{5, 2, 1, 0, 1, 1, 1, 1, 2, 0, 0, 0, 3, 3, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 6, 10,0, 0, 10,10, 0, 10,10,10, 0, 5},
		{5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 6, 10,0, 0, 0, 0, 0, 0, 0, 0, 0, 5},
		{5, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 2, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 6, 10,0, 0, 0, 0, 0, 0, 0, 0, 0, 5},
		{5, 0, 0, 0, 3, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 6, 10,0, 0, 10, 10, 0, 0, 0, 0, 0, 5},
		{5, 0, 0, 0, 3, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 6, 10,0, 0, 10, 0, 0, 0, 0, 0, 0, 5},
		{5, 0, 0, 0, 3, 1, 1, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 6, 6, 6, 6, 6, 6, 10,0, 0, 10, 0, 10, 0, 0, 0, 0, 5},
		{5, 0, 0, 0, 3, 1, 1, 3, 3, 3, 3, 3, 3, 0, 0, 2, 0, 2, 0, 2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 10,10, 10, 10, 0, 10, 0, 10, 0, 0, 5},
		{5, 0, 0, 0, 3, 2, 2, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 10,0, 0, 10, 0, 10, 0, 10, 0, 0, 5},
		{5, 1, 2, 3, 3, 2, 2, 3, 3, 3, 3, 3, 3, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,10, 10, 10, 10, 0, 0, 0, 0, 10, 0, 0, 0, 0, 5},
		{5, 1, 2, 3, 3, 2, 2, 3, 3, 3, 3, 3, 3, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5},
		{5, 0, 0, 0, 3, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5},
		{5, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5},
		{5, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5},
		{5, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5},
		{5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5},
		{10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10}
	};

	StateGL sgl = configurable_demo_12_init((byte*) heightmap, map_width, map_height);
	sgl.num_textures = 14;
	sgl.textures = init_textures(sgl.num_textures,
		"../../../assets/walls/pyramid_bricks_4.bmp",
		"../../../assets/walls/hieroglyph.bmp",
		"../../../assets/walls/mesa.bmp",
		"../../../assets/walls/dune.bmp",
		"../../../assets/walls/cobblestone.bmp",
		"../../../assets/walls/hieroglyph.bmp",
		"../../../assets/walls/saqqara.bmp",
		"../../../assets/walls/trinity.bmp",
		"../../../assets/walls/cross_blue.bmp",
		"../../../assets/walls/dirt.bmp",
		"../../../assets/walls/dial.bmp",
		"../../../assets/walls/desert_snake.bmp",
		"../../../assets/walls/greece.bmp",
		"../../../assets/walls/arthouse_bricks.bmp");

	return sgl;
}

StateGL demo_12_tpt_init(void) {
	enum {map_width = 10, map_height = 20};

	static const byte heightmap[map_height][map_width] = {
		{6, 6, 6, 6, 6, 6, 6, 6, 6, 6},
		{6, 0, 0, 0, 0, 0, 0, 6, 2, 6},
		{6, 0, 0, 0, 0, 0, 0, 6, 2, 6},
		{6, 0, 6, 6, 6, 6, 0, 6, 2, 6},
		{6, 0, 4, 3, 3, 6, 0, 6, 2, 6},
		{6, 0, 4, 4, 4, 0, 0, 6, 2, 6},
		{6, 4, 4, 4, 4, 0, 0, 6, 2, 6},
		{6, 4, 4, 4, 4, 0, 0, 6, 2, 6},
		{6, 4, 0, 0, 0, 0, 0, 6, 2, 6},
		{6, 4, 4, 0, 0, 0, 0, 6, 2, 6},
		{6, 4, 4, 0, 0, 0, 0, 6, 2, 6},
		{6, 4, 4, 0, 0, 0, 0, 6, 2, 6},
		{6, 4, 6, 6, 6, 6, 0, 6, 2, 6},
		{6, 3, 3, 3, 3, 6, 0, 6, 2, 6},
		{6, 3, 3, 3, 3, 6, 0, 6, 2, 6},
		{6, 6, 6, 6, 6, 6, 0, 6, 2, 6},
		{6, 3, 3, 3, 3, 6, 0, 6, 2, 6},
		{6, 3, 3, 3, 3, 6, 6, 6, 2, 6},
		{6, 3, 2, 2, 2, 2, 2, 2, 2, 6},
		{6, 6, 6, 6, 6, 6, 6, 6, 6, 6}
	};

	StateGL sgl = configurable_demo_12_init((byte*) heightmap, map_width, map_height);
	sgl.num_textures = 1;
	sgl.textures = init_textures(sgl.num_textures, "../../../assets/walls/pyramid_bricks_2.bmp");
	return sgl;
}

StateGL demo_12_pyramid_init(void) {
	enum {map_width = 30, map_height = 40};

	static const byte heightmap[map_height][map_width] = {
		{3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3},
		{3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3},
		{3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3},
		{3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3},
		{3, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 3},
		{3, 0, 0, 0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 3},
		{3, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 3},
		{3, 0, 0, 0, 1, 2, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 2, 1, 0, 0, 0, 3},
		{3, 0, 0, 0, 1, 2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2, 1, 0, 0, 0, 3},
		{3, 0, 0, 0, 1, 2, 3, 4, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 5, 4, 3, 2, 1, 0, 0, 0, 3},
		{3, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 3},
		{3, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 8, 8, 8, 8, 8, 8, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 3},
		{3, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 9, 9, 9, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 3},
		{3, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,10,10,10,9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 3},
		{3, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,0, 0, 10,9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 3},
		{3, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,0, 0, 10,9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 3},
		{3, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,10,10,10,9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 3},
		{3, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 9, 9, 9, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 3},
		{3, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 8, 8, 8, 8, 8, 8, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 3},
		{3, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 3},
		{3, 0, 0, 0, 1, 2, 3, 4, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 5, 4, 3, 2, 1, 0, 0, 0, 3},
		{3, 0, 0, 0, 1, 2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2, 1, 0, 0, 0, 3},
		{3, 0, 0, 0, 1, 2, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 2, 1, 0, 0, 0, 3},
		{3, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 3},
		{3, 0, 0, 0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 3},
		{3, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 3},
		{3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3},
		{3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3},
		{3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3},
		{12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12},
		{15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15},
		{15,5, 1, 2, 7, 8, 9, 10,9, 8, 7, 11,11,11,7, 7, 7, 7, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 15},
		{15,5, 1, 3, 6, 8, 9, 10,9, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 9, 9, 9, 9, 9, 0, 0, 0, 0, 0, 15},
		{15,5, 1, 3, 6, 8, 8, 10,9, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 9, 9, 9, 9, 9, 0, 0, 0, 0, 0, 15},
		{15,5, 1, 3, 6, 8, 8, 10,9, 7, 7, 7, 7, 7, 11,11,11,11,7, 9, 9, 9, 9, 9, 0, 0, 0, 0, 0, 15},
		{15,5, 1, 3, 6, 8, 8, 10,9, 7, 7, 7, 7, 7, 11,11,11,11,7, 9, 9, 9, 9, 9, 0, 0, 0, 0, 0, 15},
		{15,5, 1, 3, 6, 8, 8, 10,9, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 9, 9, 9, 9, 9, 0, 0, 0, 0, 0, 15},
		{15,5, 1, 3, 6, 8, 9, 10,9, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 9, 9, 9, 9, 9, 0, 0, 0, 0, 0, 15},
		{15,0, 1, 4, 5, 8, 9, 10,9, 8, 7, 11,11,11,7, 7, 7, 7, 7, 9, 9, 9, 9, 9, 0, 0, 0, 0, 0, 15},
		{15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15}
	};

	StateGL sgl = configurable_demo_12_init((byte*) heightmap, map_width, map_height);
	sgl.num_textures = 1;
	sgl.textures = init_textures(sgl.num_textures, "../../../assets/walls/greece.bmp");
	return sgl;
}

void demo_12_drawer(const StateGL* const sgl) {
	move(sgl -> shader_program);
	glClearColor(0.8901960784313725f, 0.8549019607843137f, 0.788235294117647f, 0.0f); // Bone

	const SectorList* const sector_list = sgl -> any_data;
	for (int i = 0; i < sector_list -> length; i++) {
		const int tex_ind = (double) i / sgl -> num_vertex_buffers * sgl -> num_textures;
		select_texture_for_use(sgl -> textures[tex_ind], sgl -> shader_program);

		glBindBuffer(GL_ARRAY_BUFFER, sgl -> vertex_buffers[i]);
		bind_interleaved_planes_to_vao();

		draw_triangles((sector_list -> data[i].area.height == 0) ? triangles_per_height_zero_mesh : triangles_per_mesh);
	}
}

void demo_12_deinit(const StateGL* const sgl) {
	const SectorList* const sector_list = sgl -> any_data;
	deinit_sector_list((*sector_list)); // This frees the internal sector data
	free(sgl -> any_data); // This frees the sector list struct on the heap

	deinit_demo_vars(sgl);
}

#ifdef DEMO_12
int main(void) {
	make_application(demo_12_drawer, demo_12_palace_init, demo_12_deinit);
}
#endif
