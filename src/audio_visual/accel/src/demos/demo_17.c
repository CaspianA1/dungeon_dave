#include "../utils.c"
#include "../sector.c"
#include "../batch.c"
#include "../camera.c"

#include "demo_11.c"

typedef struct {
	SectorList sectors;
	SectorBatch batch;
} SectorState;

StateGL demo_17_init(void) {
	StateGL sgl = {.vertex_array = init_vao()};
	sgl.num_vertex_buffers = 0;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers);

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

	SectorState* const sector_state = malloc(sizeof(SectorState));
	sector_state -> sectors = generate_sectors_from_heightmap((byte*) heightmap, map_width, map_height);
	sector_state -> batch = init_sector_batch(sector_state -> sectors.length, triangles_per_mesh, bytes_per_mesh, bytes_per_vertex, PLANE_TYPE_ENUM);

	// Now, create mesh from sectors, and store that in sectors (so adapt the old sector demos to that)

	sgl.any_data = sector_state;

	sgl.shader_program = init_shader_program(demo_4_vertex_shader, demo_4_fragment_shader);
	sgl.num_textures = 1;
	sgl.textures = init_textures(sgl.num_textures, "../../../assets/walls/hieroglyph.bmp", tex_repeating);
	select_texture_for_use(sgl.textures[0], sgl.shader_program);
	enable_all_culling();

	return sgl;
}

void demo_17_drawer(const StateGL* const sgl) {
	static Camera camera;
	static GLint model_view_projection_id;
	static byte first_call = 1;

	if (first_call) {
		init_camera(&camera, (vec3) {0.0f, 0.0f, 0.0f});
		model_view_projection_id = glGetUniformLocation(sgl -> shader_program, "model_view_projection");
		first_call = 0;
	}

	glClearColor(0.2f, 0.4f, 0.5f, 0.0f);
}

void demo_17_deinit(const StateGL* const sgl) {
	const SectorState* const sector_state = sgl -> any_data;
	deinit_sector_list(sector_state -> sectors);
	deinit_sector_batch(&sector_state -> batch);

	deinit_demo_vars(sgl);
}

#ifdef DEMO_17
int main(void) {
	make_application(demo_17_drawer, demo_17_init, demo_17_deinit);
}
#endif
