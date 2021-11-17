#include "demo_11.c"
#include "../sector.c"
#include "../maps.c"
#include "../camera.c"

/*
- Sectors contain their meshes
- To begin with, don't clip sector heights based on adjacent heights
- Sectors are rectangular

- Not perfect, but sectors + their meshes for clipping and rendering, and texmaps + heightmaps for game logic
- Ideal: BSPs, but not worth time
- To start, one vbo + texture ptr per sector
- For map edges, only render inside + top face
- Store texture byte index in a plane (max 10 textures per level)
- Frustum culling
- A little seam between some textures - but inevitable
- Lighting with normal
- Fix little dots popping around
_____
- Clip sectors based on adjacent heights
- For neighboring sectors with the same height, make them into flat 2D planes
- Or in the general case, if a plane is partially invisible, truncate it; otherwise, remove it
- Find which sectors are behind, and then skip rendering those

- Read sprite crop from spritesheet

- Blit 2D sprite to whole screen
- Blit color rect to screen
- Flat weapon
*/

// These two add distance shading from the demo 4 fragment shader
const char* const demo_12_vertex_shader =
	"#version 330 core\n"

	"layout(location = 0) in vec3 vertex_pos_world_space;\n"
	"layout(location = 1) in vec2 vertex_UV;\n"

	"out vec2 UV;\n"
	"out vec3 pos_delta_world_space;\n"

	"uniform vec3 camera_pos_world_space;\n"
	"uniform mat4 model_view_projection;\n"

	"void main() {\n"
		"gl_Position = model_view_projection * vec4(vertex_pos_world_space, 1);\n"
		"UV = vertex_UV;\n"
		"pos_delta_world_space = camera_pos_world_space - vertex_pos_world_space;\n"
	"}\n",

*const demo_12_fragment_shader =
	"#version 330 core\n"

	"in vec2 UV;\n"
	"in vec3 pos_delta_world_space;\n"

	"out vec3 color;\n"

	"uniform sampler2D texture_sampler;\n"

	"const float min_light = 0.1f, max_light = 1.0f, intensity_factor = 50.0f;\n"

	"void main() {\n" // dist_squared is distance squared from fragment
		"float dist_squared = dot(pos_delta_world_space, pos_delta_world_space);\n"
		"float light_intensity = clamp(intensity_factor / dist_squared, min_light, max_light);\n"
		"color = texture(texture_sampler, UV).rgb * light_intensity;\n"
	"}\n";

StateGL configurable_demo_12_init(byte* const heightmap, const byte map_width, const byte map_height) {
	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0};

	SectorList sector_list = generate_sectors_from_heightmap(heightmap, map_width, map_height);
	init_sector_list_vbo(&sector_list);
	bind_interleaved_planes_to_vao();

	SectorList* const sector_list_on_heap = malloc(sizeof(SectorList));
	*sector_list_on_heap = sector_list;
	sgl.any_data = sector_list_on_heap; // any_data stores sector meshes, and freed in demo_12_deinit

	sgl.shader_program = init_shader_program(demo_12_vertex_shader, demo_12_fragment_shader);
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
	sgl.num_textures = 1;
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
	draw_triangles(sector_list -> num_vertices);
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
