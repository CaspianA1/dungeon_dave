// This aims to optimize meshes when they are created, expanding upon sector_mesh.c.

#include "demo_4.c"

// #include "../sector_mesh.c"
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

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 3, MESH_TYPE_ENUM, GL_FALSE, bytes_per_vertex, NULL);
	glVertexAttribPointer(1, 2, MESH_TYPE_ENUM, GL_FALSE, bytes_per_vertex, (void*) (3 * sizeof(mesh_type_t)));

	sgl.shader_program = init_shader_program(demo_4_vertex_shader, demo_4_fragment_shader);
	sgl.num_textures = 1;
	sgl.textures = init_textures(sgl.num_textures, "../../../assets/walls/mesa.bmp", tex_repeating);
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

	SectorList s;
	init_face_and_sector_lists(&face_list_17, &s, heightmap, map_width, map_height);
	sector_list_17 = &s;

	make_application(demo_17_drawer, demo_17_init, demo_17_deinit);
}
#endif
