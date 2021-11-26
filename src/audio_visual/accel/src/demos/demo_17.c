// This aims to optimize meshes when they are created, expanding upon sector_mesh.c.

#include "demo_12.c"

typedef enum {
	/* NS - north-south, and EW = east-west.
	If a face is NS, its two ends lie on a vertical top-down axis;
	and if a face is EW, its two ends lie on a horizontal axis. */
	Flat, Vert_NS, Vert_EW
} FaceType;

typedef struct {
	const FaceType type;
	/* Faces don't store their height origin, since sectors store that.
	For vert faces, origin and size[0] are top-down, and size[1] is depth.
	For hori faces, origin and size are both top-down. */
	byte origin[2], size[2];
} Face;

void print_face(const Face face, const char* const prefix_msg) {
	const char* const type_string =
		(char*[]) {"Flat", "Vert_NS", "Vert_EW"}[face.type];

	printf("%s{.type = %s, .origin = {%d, %d}, .size = {%d, %d}}\n",
		prefix_msg, type_string, face.origin[0],
		face.origin[1], face.size[0], face.size[1]);
}

// Returns if there is anotehr face to get
byte get_next_face(const Sector sector, const byte varying_axis,
	const byte adjacent_side_val, const byte map_width,
	const byte* const heightmap, Face* const face) {
	
	int16_t face_height_diff = 0;
	const byte end_edge_val = sector.origin[varying_axis] + sector.size[varying_axis];

	byte start_val = face -> origin[varying_axis] + face -> size[0], map_point_params[2];
	map_point_params[!varying_axis] = adjacent_side_val;

	while (start_val < end_edge_val) {
		map_point_params[varying_axis] = start_val;
		const int16_t height_diff = sector.height - *map_point((byte*) heightmap,
			map_point_params[0], map_point_params[1], map_width);

		if (height_diff > 0) {
			face_height_diff = height_diff;
			break;
		}
		start_val++;
	}

	if (start_val == end_edge_val) return 0;

	byte end_val = start_val;
	while (end_val < end_edge_val) {
		map_point_params[varying_axis] = end_val;
		const int16_t height_diff = sector.height - *map_point((byte*) heightmap,
			map_point_params[0], map_point_params[1], map_width);

		if (height_diff != face_height_diff) break;
		end_val++;
	}

	face -> origin[varying_axis] = start_val;
	face -> size[0] = end_val - start_val;
	face -> size[1] = face_height_diff;

	return 1;
}

void init_vert_faces(const Sector sector, List* const vertex_list,
	const byte* const heightmap, const byte map_width, const byte map_height) {

	const byte dimensions[2] = {map_width, map_height};

	for (byte unvarying_axis = 0; unvarying_axis < 2; unvarying_axis++) {
		for (byte side = 0; side < 2; side++) {
			Face next_face = {.type = Vert_EW - unvarying_axis, .origin = {sector.origin[0], sector.origin[1]}};
			byte adjacent_side_val;

			if (side) { // Side is a top side or left side of the top-down sector
				if (next_face.origin[unvarying_axis] == 0) continue;
				adjacent_side_val = next_face.origin[unvarying_axis] - 1;
			}
			else {
				if ((next_face.origin[unvarying_axis] += sector.size[unvarying_axis]) == dimensions[unvarying_axis]) continue;
				adjacent_side_val = next_face.origin[unvarying_axis];
			}

			void add_face_mesh_to_vertex_list(const Face, const byte, List* const);

			while (get_next_face(sector, !unvarying_axis, adjacent_side_val, map_width, heightmap, &next_face)) {
				// print_face(next_face, "");
				add_face_mesh_to_vertex_list(next_face, sector.height, vertex_list);
			}
		}
	}
}

/*
2,6_____5
|     |
|     |
3_____1,4

and rotate to

_______
|     |
|     |
|_____|
*/

void add_face_mesh_to_vertex_list(const Face face, const byte sector_height, List* const vertex_list) {
	const byte near_x = face.origin[0], near_z = face.origin[1];
	const mesh_type_t* face_mesh;

	switch (face.type) {
		case Flat: {
			const byte size_x = face.size[0], size_z = face.size[1];
			const byte far_x = near_x + size_x, far_z = near_z + size_z;

			face_mesh = (mesh_type_t[vars_per_face]) { // Used to be face 5 from sector_mesh.c; now UVs flipped
				near_x, sector_height, far_z, size_x, 0,
				far_x, sector_height, near_z, 0, size_z,
				near_x, sector_height, near_z, size_x, size_z,

				near_x, sector_height, far_z, size_x, 0,
				far_x, sector_height, far_z, 0, 0,
				far_x, sector_height, near_z, 0, size_z
			};
			break;
		}
		case Vert_NS: {
			const byte size_x = face.size[0], size_y = face.size[1];
			const byte far_x = near_x + size_x, bottom_y = sector_height - size_y;

			face_mesh = (mesh_type_t[vars_per_face]) { // Face 3
				near_x, sector_height, near_z, size_x, 0,
				far_x, sector_height, near_z, 0, 0,
				near_x, bottom_y, near_z, size_x, size_y,

				far_x, sector_height, near_z, 0, 0,
				far_x, bottom_y, near_z, 0, size_y,
				near_x, bottom_y, near_z, size_x, size_y
			};
			break;
		}
		case Vert_EW: {
			const byte size_z = face.size[0], size_y = face.size[1];
			const byte far_z = near_z + size_z, bottom_y = sector_height - size_y;

			face_mesh = (mesh_type_t[vars_per_face]) { // Face 1
				near_x, bottom_y, near_z, 0, size_y,
				near_x, sector_height, far_z, size_z, 0,
				near_x, sector_height, near_z, 0, 0,

				near_x, bottom_y, near_z, 0, size_y,
				near_x, bottom_y, far_z, size_z, size_y,
				near_x, sector_height, far_z, size_z, 0
			};
			break;
		}
	}
	push_ptr_to_list(vertex_list, face_mesh);
}

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

	sgl.shader_program = init_shader_program(demo_12_vertex_shader, demo_12_fragment_shader);
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
