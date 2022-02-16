/*
- Ambient occlusion!
- This draws a sector-based map, and applies ambient occlusion to it.
*/

#include "../utils.c"
#include "../batch_draw_context.c"
#include "../sector.c"
#include "../texture.c"
#include "../camera.c"
#include "../event.c"
#include "../data/maps.c"

//////////

typedef struct {
	List sectors;
	BatchDrawContext sector_draw_context;

	const GLuint lightmap_texture; // This is grayscale

	const byte* const heightmap;
	const byte map_size[2];
} SceneState;

//////////

StateGL demo_24_init(void) {
	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0, .num_textures = 0};

	SceneState scene_state = {
		.lightmap_texture = init_plain_texture("../assets/water_grayscale.bmp", TexPlain, TexNonRepeating, OPENGL_GRAYSCALE_INTERNAL_PIXEL_FORMAT),
		.heightmap = (byte*) palace_heightmap,
		.map_size = {palace_width, palace_height},
	};

	init_sector_draw_context(&scene_state.sector_draw_context, &scene_state.sectors,
		scene_state.heightmap, (byte*) palace_texture_id_map, scene_state.map_size);
	
	scene_state.sector_draw_context.texture_set = init_texture_set(TexRepeating,
		11, 0, 128, 128,
		"../../../../assets/walls/sand.bmp", "../../../../assets/walls/pyramid_bricks_4.bmp",
		"../../../../assets/walls/marble.bmp", "../../../../assets/walls/hieroglyph.bmp",
		"../../../../assets/walls/window.bmp", "../../../../assets/walls/saqqara.bmp",
		"../../../../assets/walls/sandstone.bmp", "../../../../assets/walls/cobblestone_3.bmp",
		"../../../../assets/walls/horses.bmp", "../../../../assets/walls/mesa.bmp",
		"../../../../assets/walls/arthouse_bricks.bmp"
	);
	
	sgl.any_data = malloc(sizeof(SceneState));
	memcpy(sgl.any_data, &scene_state, sizeof(SceneState));

	enable_all_culling();
	glEnable(GL_MULTISAMPLE);

	return sgl;
}

void demo_24_drawer(const StateGL* const sgl) {
	const SceneState* const scene_state = (SceneState*) sgl -> any_data;

	static Camera camera;
	static byte first_call = 1;

	if (first_call) {
		init_camera(&camera, (vec3) {1.5f, 1.5f, 1.5f});
		first_call = 0;
	}

	update_camera(&camera, get_next_event(), NULL);

	glClearColor(0.4f, 0.4f, 0.4f, 0.0f); // Light gray
	draw_visible_sectors(&scene_state -> sector_draw_context, &scene_state -> sectors,
		&camera, scene_state -> lightmap_texture, scene_state -> map_size);
}

void demo_24_deinit(const StateGL* const sgl) {
	const SceneState* const scene_state = (SceneState*) sgl -> any_data;

	deinit_texture(scene_state -> lightmap_texture);
	deinit_batch_draw_context(&scene_state -> sector_draw_context);
	deinit_list(scene_state -> sectors);
	free(sgl -> any_data);

	deinit_demo_vars(sgl);
}

#ifdef DEMO_24
int main(void) {
	make_application(demo_24_drawer, demo_24_init, demo_24_deinit);
}
#endif
