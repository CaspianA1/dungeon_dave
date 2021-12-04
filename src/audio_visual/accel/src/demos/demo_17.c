#include "demo_4.c"
#include "../sector.c"
#include "../face.c"
#include "../camera.c"
#include "../data/maps.c"
#include "../culling.c"
#include "../texture.c"

/*
- NEXT: new_map
- NEXT 2: different textures for a map
- NEXT 3: a bounding volume hierarchy, maybe
- NEXT 4: Composable drawers - can just call draw_sectors_in_view_frustum and draw_billboards in one call

- Point light sources, or simple lightmaps
- Store the cpu index list in three-bit parts; bit 0 = vert or flat, bit 1 = ns or ew, and bit 2 = side

- Read sprite crop from spritesheet
- Blit 2D sprite to whole screen
- Blit color rect to screen
- Flat weapon

- In the end, 5 shaders + accel components: sectors, billboards, skybox, weapon, ui elements
*/

StateGL demo_17_init(void) {
	/*
	for (byte y = 0; y < terrain_height; y++) {
		for (byte x = 0; x < terrain_width; x++) {
			*map_point((byte*) terrain_map, x, y, terrain_width) = fabsf((cosf(x / 5.0f) + sinf(y / 5.0f))) * 5.0f;
			// *map_point((byte*) terrain_map, x, y, terrain_width) *= ((x + y) >> 1) / 50.0f;
		}
	}
	*/

	/*
	tiny_map, tiny_width, tiny_height
	palace_map, palace_width, palace_height
	terrain_map, terrain_width, terrain_height
	tpt_map, tpt_width, tpt_height
	new_map, new_width, new_height
	*/

	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0};
	List face_mesh_list;
	SectorList sector_list;

	init_face_mesh_and_sector_lists(&sector_list, &face_mesh_list, (byte*) new_map, new_width, new_height);
	init_sector_list_vbo_and_ibo(&sector_list, &face_mesh_list);
	bind_sector_list_vbo_to_vao(&sector_list);

	SectorList* const sector_list_on_heap = malloc(sizeof(SectorList));
	*sector_list_on_heap = sector_list;
	sgl.any_data = sector_list_on_heap;

	sgl.shader_program = init_shader_program(sector_vertex_shader, sector_fragment_shader);
	glUseProgram(sgl.shader_program);

	//////////
	sgl.num_textures = 0;
	const GLuint ts = init_texture_set(TexRepeating, 64, 64, 1,
		"../../../assets/walls/dune.bmp");
		// "../../../assets/walls/mesa.bmp",
		// "../../../assets/walls/hieroglyph.bmp");

	use_texture(ts, sgl.shader_program, TexSet);

	enable_all_culling();
	glEnable(GL_MULTISAMPLE);

	deinit_list(face_mesh_list);

	return sgl;
}

void demo_17_drawer(const StateGL* const sgl) {
	static Camera camera;
	static GLint camera_pos_id, model_view_projection_id;
	static byte first_call = 1;

	if (first_call) { // start new map: 1.5f, 0.5f, 1.5f
		init_camera(&camera, (vec3) {28.0f, 20.0f, 24.0f}); // terrain: 34.5f, 13.50f, 25.2f
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

	const SectorList* const sector_list = (SectorList*) sgl -> any_data;
	draw_sectors_in_view_frustum(sector_list, &camera);
}

void demo_17_deinit(const StateGL* const sgl) {
	const SectorList* const sector_list = (SectorList*) sgl -> any_data;
	deinit_sector_list(sector_list);
	free(sgl -> any_data);

	deinit_demo_vars(sgl);
}

#ifdef DEMO_17
int main(void) {
	make_application(demo_17_drawer, demo_17_init, demo_17_deinit);
}
#endif
