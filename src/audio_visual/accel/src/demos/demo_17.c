#include "demo_4.c"
#include "../sector.c"
#include "../face.c"
#include "../camera.c"
#include "../maps.c"

static List fl;
static SectorList sl;

StateGL demo_17_init(void) {
	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0};

	init_face_and_sector_lists(&fl, &sl, (byte*) palace_map, palace_width, palace_height);
	init_sector_list_vbo(&fl, &sl);
	bind_sector_list_vbo_to_vao(&sl);

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

	// (triangle counts, 12 vs 17) palace: 1466 vs 1114. tpt: 232 vs 146. pyramid: 816 vs 542. maze: 5796 vs 6114.
	const GLsizei num_triangles = fl.length * triangles_per_face;
	draw_triangles(num_triangles);
}

void demo_17_deinit(const StateGL* const sgl) {
	deinit_sector_list(&sl);
	deinit_list(fl);
	deinit_demo_vars(sgl);
}

#ifdef DEMO_17
int main(void) {
	make_application(demo_17_drawer, demo_17_init, demo_17_deinit);
}
#endif