typedef GLubyte mesh_type_t;
#define MESH_TYPE_ENUM GL_UNSIGNED_BYTE

enum {
	vars_per_vertex = 5,
	vertices_per_triangle = 3,
	triangles_per_face = 2,
	triangles_per_mesh = 10, // Bottom 2 triangles excluded

	bytes_per_vertex = vars_per_vertex * sizeof(mesh_type_t),
	vars_per_triangle = vars_per_vertex * vertices_per_triangle,

	vars_per_face = vars_per_triangle * triangles_per_face,
	vars_per_mesh = vars_per_triangle * triangles_per_mesh,

	bytes_per_mesh = vars_per_mesh * sizeof(mesh_type_t),
	bytes_per_height_zero_mesh = vars_per_face * sizeof(mesh_type_t)
};


void check_for_mesh_out_of_bounds(const mesh_type_t origin[3], const mesh_type_t size[3]) {
	for (byte i = 0; i < 3; i++) {
		const int start = origin[i], length = size[i];
		const int end = start + ((i == 1) ? -length : length);

		if (start < 0 || start > 255 || end < 0 || end > 255) {
			fprintf(stderr, "Mesh out of bounds on %c axis\n", 'x' + i);
			fail("create mesh: mesh out of bounds", MeshOutOfBounds);
		}
	}
}

/* Even if normal sector meshes can represent height zero meshes, it's worth it to make a separate type
mesh for when a sector has height 0 because I save 120 bytes of memory that way (480 with floats!) */
void create_height_zero_mesh(const mesh_type_t origin[3], const mesh_type_t size[2], mesh_type_t* const dest) {
	check_for_mesh_out_of_bounds(origin, (mesh_type_t[3]) {size[0], 0, size[1]});

	const mesh_type_t
		size_x = size[0], size_z = size[1],
		near_x = origin[0], top_y = origin[1], near_z = origin[2];

	const mesh_type_t far_x = near_x + size_x, far_z = near_z + size_z;

	const mesh_type_t vertices[vars_per_face] = {
		near_x, top_y, far_z, size_z, size_x,
		far_x, top_y, near_z, 0, 0,
		near_x, top_y, near_z, 0, size_x,

		near_x, top_y, far_z, size_z, size_x,
		far_x, top_y, far_z, size_z, 0,
		far_x, top_y, near_z, 0, 0
	};

	memcpy(dest, vertices, bytes_per_height_zero_mesh);
}

void create_sector_mesh(const mesh_type_t origin[3], const mesh_type_t size[3], mesh_type_t* const dest) {
	check_for_mesh_out_of_bounds(origin, size);

	const mesh_type_t
		near_x = origin[0], top_y = origin[1], near_z = origin[2],
		size_x = size[0], size_y = size[1], size_z = size[2];

	const mesh_type_t far_x = near_x + size_x, bottom_y = top_y - size_y, far_z = near_z + size_z;

	// Side descriptions assume that camera direction is aligned to X axis
	const mesh_type_t vertices[vars_per_mesh] = {
		// Face 1, pointing towards Z, near X, front face
		near_x, bottom_y, near_z, 0, size_y,
		near_x, top_y, far_z, size_z, 0,
		near_x, top_y, near_z, 0, 0,

		near_x, bottom_y, near_z, 0, size_y,
		near_x, bottom_y, far_z, size_z, size_y,
		near_x, top_y, far_z, size_z, 0,

		// Face 2, pointing towards Z, far X, back face
		far_x, top_y, near_z, size_z, 0,
		far_x, top_y, far_z, 0, 0,
		far_x, bottom_y, near_z, size_z, size_y,

		far_x, top_y, far_z, 0, 0,
		far_x, bottom_y, far_z, 0, size_y,
		far_x, bottom_y, near_z, size_z, size_y,

		// Face 3, pointing towards X, left face
		near_x, top_y, near_z, size_x, 0,
		far_x, top_y, near_z, 0, 0,
		near_x, bottom_y, near_z, size_x, size_y,

		far_x, top_y, near_z, 0, 0,
		far_x, bottom_y, near_z, 0, size_y,
		near_x, bottom_y, near_z, size_x, size_y,

		// Face 4, pointing towards X, right face
		near_x, bottom_y, far_z, 0, size_y,
		far_x, top_y, far_z, size_x, 0,
		near_x, top_y, far_z, 0, 0,

		near_x, bottom_y, far_z, 0, size_y,
		far_x, bottom_y, far_z, size_x, size_y,
		far_x, top_y, far_z, size_x, 0,

		// Face 5, top flat face
		near_x, top_y, far_z, size_z, size_x,
		far_x, top_y, near_z, 0, 0,
		near_x, top_y, near_z, 0, size_x,

		near_x, top_y, far_z, size_z, size_x,
		far_x, top_y, far_z, size_z, 0,
		far_x, top_y, near_z, 0, 0
	};

	memcpy(dest, vertices, bytes_per_mesh);
}

void bind_sector_mesh_to_vao(void) {
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	/*
	glVertexAttribIPointer(0, 3, MESH_TYPE_ENUM, bytes_per_vertex, NULL);
	glVertexAttribIPointer(1, 2, MESH_TYPE_ENUM, bytes_per_vertex, (void*) (3 * sizeof(mesh_type_t)));
	*/

	glVertexAttribPointer(0, 3, MESH_TYPE_ENUM, GL_FALSE, bytes_per_vertex, NULL);
	glVertexAttribPointer(1, 2, MESH_TYPE_ENUM, GL_FALSE, bytes_per_vertex, (void*) (3 * sizeof(mesh_type_t)));
}
