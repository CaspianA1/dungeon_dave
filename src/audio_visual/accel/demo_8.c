#include "demo_7.c"

/*
- optimized plane drawing
- want a solid base before continuing
- draw simple vert plane with GL_TRIANGLE_STRIP, or use indices if possible
- now interleave UV
*/

/*
0__1
|  /
| /
2/
*/

enum {interleaved_plane_floats = 20};
enum {interleaved_plane_bytes = interleaved_plane_floats * sizeof(GLfloat)};

void create_vert_plane_interleaved(const ivec3 top_left_corner,
	const int width, const int height, GLfloat* const plane_buffer) {

	const GLfloat left_x = top_left_corner[0], top_y = top_left_corner[1], z = top_left_corner[2];
	const GLfloat right_x = left_x + width, bottom_y = top_y - height;

	const GLfloat vertices_with_UV[interleaved_plane_floats] = {
		left_x, top_y, z, 0.0f, 0.0f, // last two floats are UV
		right_x, top_y, z, width, 0.0f,

		left_x, bottom_y, z, 0.0f, height,
		right_x, bottom_y, z, width, height
	};

	memcpy(plane_buffer, vertices_with_UV, interleaved_plane_bytes);
}

StateGL demo_8_init(void) {
	StateGL sgl;
	sgl.vertex_array = init_vao();

	enum {width = 3, height = 5};
	const ivec3 top_left_corner = {4, 4, 4};

	GLfloat* const plane_data = malloc(interleaved_plane_bytes);
	create_vert_plane_interleaved(top_left_corner, width, height, plane_data);

	sgl.num_vertex_buffers = 1;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers,
		plane_data, interleaved_plane_bytes);

	free(plane_data);
	demo_6_core_init_shader_and_textures_and_culling(&sgl);
	return sgl;
}

void demo_8_drawer(const StateGL sgl);

#ifdef DEMO_8
int main(void) {

}
#endif
