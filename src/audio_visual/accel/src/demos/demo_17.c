#include "demo_4.c"
#include "../sector.c"
#include "../face.c"
#include "../camera.c"
#include "../data/maps.c"
#include "../culling.c"

/*
- Sectors contain their meshes
- To begin with, don't clip sector heights based on adjacent heights
- Sectors are rectangular

- Not perfect, but sectors + their meshes for clipping and rendering, and texmaps + heightmaps for game logic
- Ideal: BSPs, but not worth time
- To start, one vbo + texture ptr per sector

- Store texture byte index in a plane (max 10 textures per level)
- NEXT: copy visible sectors in chunks
- NEXT 2: a bounding volume hierarchy
- NEXT 3: a draw_sectors function, which will allow for skybox + sector drawers together
- A little seam between some textures + little dots popping around - find a way to share vertices, if possible - only happens/seen when it's dark?
- Maybe no real-time lighting (only via lightmaps); excluding distance lighting
- Only very simple lighting with ambient and diffuse (those should handle distance implicitly) + simple lightmaps
- Find out why demo 12 uses so much less gpu % than demo 17

- Read sprite crop from spritesheet
- Blit 2D sprite to whole screen
- Blit color rect to screen
- Flat weapon

- In the end, 5 shaders + accel components: sectors, billboards, skybox, weapon, ui elements
*/

static SectorList sl;

StateGL demo_17_init(void) {
	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0};

	static List face_mesh_list;
	init_face_mesh_and_sector_lists(&sl, &face_mesh_list, (byte*) terrain_map, terrain_width, terrain_height);

	init_sector_list_vbo_and_ibo(&sl, &face_mesh_list);
	bind_sector_list_vbo_to_vao(&sl);

	sgl.shader_program = init_shader_program(sector_vertex_shader, sector_fragment_shader);
	glUseProgram(sgl.shader_program);
	sgl.num_textures = 1;
	sgl.textures = init_textures(sgl.num_textures, "../../../assets/walls/dirt.bmp", tex_repeating);
	select_texture_for_use(sgl.textures[0], sgl.shader_program);

	enable_all_culling();
	deinit_list(face_mesh_list);

	return sgl;
}

void demo_17_drawer(const StateGL* const sgl) {
	static Camera camera;
	static GLint camera_pos_id, model_view_projection_id;
	static byte first_call = 1;

	if (first_call) {
		init_camera(&camera, (vec3) {1.0f, 1.0f, 1.0f}); // terrain: 34.5f, 13.50f, 25.2f
		camera_pos_id = glGetUniformLocation(sgl -> shader_program, "camera_pos_world_space");
		model_view_projection_id = glGetUniformLocation(sgl -> shader_program, "model_view_projection");
		first_call = 0;
	}

	update_camera(&camera);

	glUniform3f(camera_pos_id, camera.pos[0], camera.pos[1], camera.pos[2]);
	glUniformMatrix4fv(model_view_projection_id, 1, GL_FALSE, &camera.model_view_projection[0][0]);

	glClearColor(0.89f, 0.355f, 0.288f, 0.0f); // Light tomato

	/* (triangle counts, 12 vs 17)
	palace: 1466 vs 1114. tpt: 232 vs 146.
	pyramid: 816 vs 542. maze: 5796 vs 6114.
	terrain: 150620 vs 86588. */

	draw_sectors_in_view_frustum(&sl, &camera);
}

void demo_17_deinit(const StateGL* const sgl) {
	deinit_sector_list(&sl);
	deinit_demo_vars(sgl);
}

#ifdef DEMO_17
int main(void) {
	make_application(demo_17_drawer, demo_17_init, demo_17_deinit);
}
#endif
