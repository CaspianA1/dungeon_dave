// This aims to optimize meshes when they are created, expanding upon sector_mesh.c.

#include "demo_4.c"

#include "../sector_mesh.c"
#include "../sector.c"
#include "../face.c"

#include "../camera.c"
#include "../maps.c"

//////////

static List face_list_17;
static SectorList* sector_list_17;

StateGL demo_17_init(void) {
	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0};

	const size_t total_triangles = face_list_17.length * triangles_per_face;
	const size_t total_bytes = total_triangles * vertices_per_triangle * bytes_per_vertex;

	glGenBuffers(1, &sector_list_17 -> vbo);
	glBindBuffer(GL_ARRAY_BUFFER, sector_list_17 -> vbo);
	glBufferData(GL_ARRAY_BUFFER, total_bytes, face_list_17.data, GL_STATIC_DRAW);
	bind_sector_mesh_to_vao();

	sgl.shader_program = init_shader_program(demo_4_vertex_shader, demo_4_fragment_shader);
	sgl.num_textures = 1;
	sgl.textures = init_textures(sgl.num_textures, "../../../assets/walls/tmr.bmp", tex_repeating);
	select_texture_for_use(sgl.textures[0], sgl.shader_program);

	enable_all_culling();

	return sgl;
}

void demo_17_drawer(const StateGL* const sgl) {
	static Camera camera;
	static GLint camera_pos_id, model_view_projection_id;
	static byte first_call = 1;

	if (first_call) {
		init_camera(&camera, (vec3) {1.5f, 0.5f, 1.5f});
		camera_pos_id = glGetUniformLocation(sgl -> shader_program, "camera_pos_world_space");
		model_view_projection_id = glGetUniformLocation(sgl -> shader_program, "model_view_projection");
		first_call = 0;
	}

	update_camera(&camera);

	glUniform3f(camera_pos_id, camera.pos[0], camera.pos[1], camera.pos[2]);
	glUniformMatrix4fv(model_view_projection_id, 1, GL_FALSE, &camera.model_view_projection[0][0]);

	glClearColor(0.89f, 0.355f, 0.288f, 0.0f); // Light tomato

	// (triangle counts, 12 vs 17) palace: 1466 vs 1128. tpt: 232 vs 156. pyramid: 816 vs 562. maze: 5796 vs 5886.
	const GLsizei num_triangles = face_list_17.length * triangles_per_face;
	draw_triangles(num_triangles);
}

void demo_17_deinit(const StateGL* const sgl) {
	deinit_sector_list(sector_list_17);
	deinit_list(face_list_17);
	deinit_demo_vars(sgl);
}

#ifdef DEMO_17
int main(void) {
	enum {map_width = palace_width, map_height = palace_height};
	const byte* const heightmap = (byte*) palace_map;

	SectorList sector_list = generate_sectors_from_heightmap((byte*) heightmap, map_width, map_height);
	List face_list = init_list(sector_list.list.length * 1.8f, mesh_type_t[vars_per_face]);

	for (size_t i = 0; i < sector_list.list.length; i++) {
		const Sector sector = ((Sector*) sector_list.list.data)[i];
		const Face flat_face = {Flat, {sector.origin[0], sector.origin[1]}, {sector.size[0], sector.size[1]}};
		add_face_mesh_to_vertex_list(flat_face, sector.height, &face_list);
		init_vert_faces(sector, &face_list, heightmap, map_width, map_height);
	}

	face_list_17 = face_list;
	sector_list_17 = &sector_list;
	make_application(demo_17_drawer, demo_17_init, demo_17_deinit);
}
#endif
