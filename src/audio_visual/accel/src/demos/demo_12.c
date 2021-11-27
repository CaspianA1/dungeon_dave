#include "demo_11.c"
#include "../sector.c"
#include "../maps.c"

/*
- Sectors contain their meshes
- To begin with, don't clip sector heights based on adjacent heights
- Sectors are rectangular

- Not perfect, but sectors + their meshes for clipping and rendering, and texmaps + heightmaps for game logic
- Ideal: BSPs, but not worth time
- To start, one vbo + texture ptr per sector

- Store texture byte index in a plane (max 10 textures per level)
- NEXT: a draw_sectors function, which will allow for skybox + sector drawers together
- NEXT 2: Frustum culling
- A little seam between some textures + little dots popping around - find a way to share vertices, if possible
- Maybe no real-time lighting (only via lightmaps); excluding distance lighting
- Only very simple lighting with ambient and diffuse (those should handle distance implicitly) + simple lightmaps

- Read sprite crop from spritesheet
- Blit 2D sprite to whole screen
- Blit color rect to screen
- Flat weapon

- In the end, 5 shaders + accel components: sectors, billboards, skybox, weapon, ui elements
*/

void init_sector_list_vbo(SectorList* const sector_list) {
	const List list = sector_list -> list;

	size_t total_bytes = 0, total_components = 0;

	for (size_t i = 0; i < list.length; i++) {
		const byte height = ((Sector*) list.data)[i].height;
		total_bytes += (height == 0) ? bytes_per_face : bytes_per_mesh;
	}

	mesh_type_t* const vertices = malloc(total_bytes);

	for (size_t i = 0; i < list.length; i++) {
		const Sector sector = ((Sector*) list.data)[i];
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

StateGL configurable_demo_12_init(byte* const heightmap, const byte map_width, const byte map_height) {
	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0};

	SectorList sector_list = generate_sectors_from_heightmap(heightmap, map_width, map_height);
	init_sector_list_vbo(&sector_list);
	bind_sector_mesh_to_vao();

	SectorList* const sector_list_on_heap = malloc(sizeof(SectorList));
	*sector_list_on_heap = sector_list;
	sgl.any_data = sector_list_on_heap; // any_data stores sector meshes, and freed in demo_12_deinit

	sgl.shader_program = init_shader_program(sector_lighting_vertex_shader, sector_lighting_fragment_shader);
	glUseProgram(sgl.shader_program);
	enable_all_culling();

	return sgl;	
}

StateGL demo_12_palace_init(void) {
	StateGL sgl = configurable_demo_12_init((byte*) palace_map, palace_width, palace_height);
	sgl.num_textures = 1; // 14
	sgl.textures = init_textures(sgl.num_textures,
		"../../../assets/walls/pyramid_bricks_3.bmp", tex_repeating);
		/*
		"../../../assets/walls/pyramid_bricks_4.bmp", tex_repeating,
		"../../../assets/walls/hieroglyph.bmp", tex_repeating,
		"../../../assets/walls/mesa.bmp", tex_repeating,
		"../../../assets/walls/dune.bmp", tex_repeating,
		"../../../assets/walls/cobblestone.bmp", tex_repeating,
		"../../../assets/walls/hieroglyph.bmp", tex_repeating,
		"../../../assets/walls/saqqara.bmp", tex_repeating,
		"../../../assets/walls/trinity.bmp", tex_repeating,
		"../../../assets/walls/cross_blue.bmp", tex_repeating,
		"../../../assets/walls/dirt.bmp", tex_repeating,
		"../../../assets/walls/dial.bmp", tex_repeating,
		"../../../assets/walls/desert_snake.bmp", tex_repeating,
		"../../../assets/walls/greece.bmp", tex_repeating,
		"../../../assets/walls/arthouse_bricks.bmp", tex_repeating);
		*/

	return sgl;
}

StateGL demo_12_tpt_init(void) {
	StateGL sgl = configurable_demo_12_init((byte*) tpt_map, tpt_width, tpt_height);
	sgl.num_textures = 1;
	sgl.textures = init_textures(sgl.num_textures, "../../../assets/walls/pyramid_bricks_2.bmp", tex_repeating);
	return sgl;
}

StateGL demo_12_pyramid_init(void) {
	StateGL sgl = configurable_demo_12_init((byte*) pyramid_map, pyramid_width, pyramid_height);
	sgl.num_textures = 1;
	sgl.textures = init_textures(sgl.num_textures, "../../../assets/walls/greece.bmp", tex_repeating);
	return sgl;
}

StateGL demo_12_maze_init(void) {
	StateGL sgl = configurable_demo_12_init((byte*) maze_map, maze_width, maze_height);
	sgl.num_textures = 1; // ivy
	sgl.textures = init_textures(sgl.num_textures, "../../../assets/walls/ivy.bmp", tex_repeating);
	return sgl;
}

void demo_12_drawer(const StateGL* const sgl) {
	static Camera camera;
	static GLint camera_pos_id, model_view_projection_id;
	static byte first_call = 1;

	if (first_call) {
		init_camera(&camera, (vec3) {1.5f, 1.0f, 1.5f});
		camera_pos_id = glGetUniformLocation(sgl -> shader_program, "camera_pos_world_space");
		model_view_projection_id = glGetUniformLocation(sgl -> shader_program, "model_view_projection");
		first_call = 0;
	}

	update_camera(&camera);

	glUniform3f(camera_pos_id, camera.pos[0], camera.pos[1], camera.pos[2]);
	glUniformMatrix4fv(model_view_projection_id, 1, GL_FALSE, &camera.model_view_projection[0][0]);

	glClearColor(0.89f, 0.855f, 0.788f, 0.0f); // Bone
	select_texture_for_use(sgl -> textures[0], sgl -> shader_program);

	const SectorList* const sector_list = sgl -> any_data;
	draw_triangles(sector_list -> num_vertices / 3);
}

void demo_12_deinit(const StateGL* const sgl) {
	const SectorList* const sector_list = sgl -> any_data;
	deinit_sector_list(sector_list); // This frees the stored sector meshes + their vbo
	free(sgl -> any_data); // This frees the sector list struct on the heap

	deinit_demo_vars(sgl);
}

#ifdef DEMO_12
int main(void) {
	make_application(demo_12_drawer, demo_12_palace_init, demo_12_deinit);
}
#endif
