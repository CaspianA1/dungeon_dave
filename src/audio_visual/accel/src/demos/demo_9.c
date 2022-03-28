#include "demo_8.c"

/*
for 0 1 2 3 4 5, drawn as {0 1 2} {1 2 3} {2 3 4} {3 4 5}

for one plane, I have 0 1 2 3, drawn as {0 1 2} {1 2 3}
for two planes, I have 0 1 2 3 4 5 6 7, needed as {0 1 2} {1 2 3} {4 5 6} {5 6 7}

to join two planes, format vertices as 0 1 2 3 3 4 4 5 6 7
drawElements(TRIANGLE_STRIP, [A, B, C, D, D, E, E, F, G, H]);
out of that, I get rects [{0 1 2} {1 2 3}] [{2 3 3} {3 3 4}] [{3 4 4} {4 4 5}] [{4 5 6} {5 6 7}]

0__1
|  /
| /
2/

    2
   /|
  / |
1/__3
*/

/* tried to join arbitrarily. didn't work at first. try with adjacent ones first.
no, will not work in all situations, b/c some have different heights, and UVs will be flipped.
*/

void print_vert(const PLANE_TYPE* const v) {
	printf("{%lf, %lf, %lf, %lf, %lf}\n", (double) v[0], (double) v[1], (double) v[2], (double) v[3], (double) v[4]);
}

enum {vars_per_vertex = 5, num_vertices_for_2 = 10};
enum {joined_2_bytes = num_vertices_for_2 * vars_per_vertex * sizeof(PLANE_TYPE)};
PLANE_TYPE* join_planes(PLANE_TYPE* const plane_1, PLANE_TYPE* const plane_2) {
	PLANE_TYPE* const joined = malloc(joined_2_bytes);

	#define set_vert(src, src_vert_ind, dest_vert_ind)\
		memcpy(joined + dest_vert_ind * vars_per_vertex, src + src_vert_ind * vars_per_vertex, vars_per_vertex * sizeof(PLANE_TYPE))

	for (int i = 0; i <= 3; i++) {
		set_vert(plane_2, i, (i + 6));
		set_vert(plane_1, i, i);
	}
	set_vert(plane_1, 3, 4);
	// set_vert(plane_2, 0, 5);

	// as {0 1 2 3 | 4 4 | 4 5 6 7}

	// {0 1 2} {1 2 3}
	// {0 1 2} {1 2 3} {2 3 4}
	// {4 5 6} {5 6 7}

	// 1. can't make it work, and 2. degenerate vertices will slow stuff down. Normal GL_TRIANGLES will be better with ints.

	puts("Plane 1:");
	for (int i = 0; i < 4; i++) print_vert(plane_1 + i * vars_per_vertex);
	puts("---\nPlane 2:");
	for (int i = 0; i < 4; i++) print_vert(plane_2 + i * vars_per_vertex);
	puts("---\nJoined:");
	for (int i = 0; i < 10; i++) print_vert(joined + i * vars_per_vertex);

	// memcpy(joined, plane_1, interleaved_plane_bytes); // A, B, C, D

	/*
	memcpy(joined, plane_1, interleaved_plane_bytes); // A, B, C, D
	memcpy(joined + 4 * vars_per_vertex, plane_1, sizeof(PLANE_TYPE)); // D
	memcpy(joined + 5 * vars_per_vertex, plane_2, sizeof(PLANE_TYPE)); // E
	memcpy(joined + 6 * vars_per_vertex, plane_2, interleaved_plane_bytes);
	*/

	return joined;
}

StateGL demo_9_init(void) {
	StateGL sgl = {.vertex_array = init_vao()};

	const PLANE_TYPE corner_1[3] = {1, 1, 1}, corner_2[3] = {2, 2, 5};
	PLANE_TYPE *const v1 = malloc(interleaved_plane_bytes), *const v2 = malloc(interleaved_plane_bytes);
	create_vert_plane_interleaved_1(corner_1, 2, 5, v1);
	create_vert_plane_interleaved_1(corner_2, 8, 9, v2);
	PLANE_TYPE* const v3 = join_planes(v1, v2);

	sgl.num_vertex_buffers = 1;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers, v3, joined_2_bytes);

	free(v1);
	free(v2);
	free(v3);

	sgl.shader = init_shader(demo_4_vertex_shader, demo_4_fragment_shader);
	use_shader(sgl.shader);
	sgl.num_textures = 1;

	sgl.num_textures = 1;
	sgl.textures = init_plain_textures(sgl.num_textures, "../../../../assets/walls/greece.bmp", TexRepeating);
	use_texture(sgl.textures[0], sgl.shader, "texture_sampler", TexPlain, 0);

	enable_all_culling();
	glClearColor(0.4f, 0.0f, 0.0f, 0.0f); // Dark blue

	return sgl;
}

void demo_9_drawer(const StateGL* const sgl) {
	move(sgl -> shader);

	enum {interleaved_vertex_bytes = 5 * sizeof(PLANE_TYPE), num_planes = 2};

	WITH_VERTEX_ATTRIBUTE(false, 0, 3, PLANE_TYPE_ENUM, interleaved_vertex_bytes, 0,
		WITH_VERTEX_ATTRIBUTE(false, 1, 2, PLANE_TYPE_ENUM, interleaved_vertex_bytes, 3 * sizeof(PLANE_TYPE),
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4 * num_planes);
		);
	);
}

#ifdef DEMO_9
int main(void) {
	make_application(demo_9_drawer, demo_9_init, deinit_demo_vars);
}
#endif
