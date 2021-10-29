#include "demo_8.c"

StateGL demo_9_init(void) {
	StateGL sgl = {.vertex_array = init_vao()};

	const PLANE_TYPE corner[3] = {0, 0, 0};

	PLANE_TYPE* const plane_buffer = malloc(interleaved_plane_bytes);
	create_hori_plane_interleaved(corner, 3, 4, plane_buffer);
	
	sgl.num_vertex_buffers = 1;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers, plane_buffer, interleaved_plane_bytes);
	bind_interleaved_planes_to_vao();
	free(plane_buffer);

	sgl.shader_program = init_shader_program(demo_4_vertex_shader, demo_4_fragment_shader);
	sgl.num_textures = 1;
	sgl.textures = init_textures(sgl.num_textures, "../../../assets/walls/greece.bmp");
	select_texture_for_use(sgl.textures[0], sgl.shader_program);
	enable_all_culling();

	return sgl;
}

#ifdef DEMO_9
int main(void) {

}
#endif
