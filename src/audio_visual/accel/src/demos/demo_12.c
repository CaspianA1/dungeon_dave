#include "demo_11.c"

#include "../list.c"
#include "../data/maps.c"

//////////

#define wmalloc malloc
#define wfree free

typedef struct {
	unsigned chunk_dimensions[2];
	size_t alloc_bytes;
	byte* data;
} StateMap;

#include "../../../../main/statemap.c"

//////////

const GLchar *const sector_lighting_vertex_shader =
    "#version 330 core\n"

	"layout(location = 0) in vec3 vertex_pos_world_space;\n"
	"layout(location = 1) in vec2 vertex_UV;\n"

	"out vec2 UV;\n"
	"out vec3 pos_delta_world_space;\n"

	"uniform vec3 camera_pos_world_space;\n"
	"uniform mat4 model_view_projection;\n"

	"void main(void) {\n"
		"gl_Position = model_view_projection * vec4(vertex_pos_world_space, 1.0f);\n"
		"UV = vertex_UV;\n"
		"pos_delta_world_space = camera_pos_world_space - vertex_pos_world_space;\n"
	"}\n",

*const sector_lighting_fragment_shader =
    "#version 330 core\n"

	"in vec2 UV;\n"
	"in vec3 pos_delta_world_space;\n"

	"out vec3 color;\n"

	"uniform sampler2D texture_sampler;\n"

	"const float min_light = 0.1f, max_light = 1.0f, intensity_factor = 50.0f;\n"

	"void main(void) {\n" // dist_squared is distance squared from fragment
		"float dist_squared = dot(pos_delta_world_space, pos_delta_world_space);\n"
		"float light_intensity = clamp(intensity_factor / dist_squared, min_light, max_light);\n"

		"color = light_intensity * texture(texture_sampler, UV).rgb;\n"
	"}\n";

//////////

typedef struct {
	List list;
	GLuint vbo, ibo;
	GLsizei num_vertices;
} OldSectorList;

typedef struct {
	const byte height, origin[2];
	byte size[2];
} OldSector;

OldSectorList init_sector_list(void) {
	return (OldSectorList) {
		.list = init_list(20, OldSector),
		.num_vertices = 0
	};
}

void deinit_sector_list(const OldSectorList* const s) {
	glDeleteBuffers(1, &s -> vbo);
	deinit_list(s -> list);
}

// Gets length across, and then adds to area size y until out of map or length across not eq
OldSector form_sector_area(OldSector sector, const StateMap traversed_points,
	const byte* const map, const byte map_width, const byte map_height) {

	byte top_right_corner = sector.origin[0];

	while (top_right_corner < map_width
		&& *map_point((byte*) map, top_right_corner, sector.origin[1], map_width) == sector.height
		&& !get_statemap_bit(traversed_points, top_right_corner, sector.origin[1])) {

		sector.size[0]++;
		top_right_corner++;
	}

	// Now, area.size[0] equals the first horizontal length of equivalent height found
	for (byte y = sector.origin[1]; y < map_height; y++, sector.size[1]++) {
		for (byte x = sector.origin[0]; x < top_right_corner; x++) {
			// If consecutive heights didn't continue
			if (*map_point((byte*) map, x, y, map_width) != sector.height)
				goto clear_map_area;
		}
	}

	clear_map_area:

	for (byte y = sector.origin[1]; y < sector.origin[1] + sector.size[1]; y++) {
		for (byte x = sector.origin[0]; x < sector.origin[0] + sector.size[0]; x++)
			set_statemap_bit(traversed_points, x, y);
	}

	return sector;
}

OldSectorList generate_sectors_from_heightmap(const byte* const heightmap,
	const byte map_width, const byte map_height) {

	OldSectorList sector_list = init_sector_list();

	/* StateMap used instead of copy of heightmap with null map points, b/c 1. less bytes used
	and 2. for forming faces, will need original heightmap to be unmodified */
	const StateMap traversed_points = init_statemap(map_width, map_height);

	for (byte y = 0; y < map_height; y++) {
		for (byte x = 0; x < map_width; x++) {
			if (get_statemap_bit(traversed_points, x, y)) continue;

			const byte height = *map_point((byte*) heightmap, x, y, map_width);
			const OldSector seed_area = {.height = height, .origin = {x, y}, .size = {0, 0}};
			const OldSector expanded_area = form_sector_area(seed_area, traversed_points, heightmap, map_width, map_height);
			push_ptr_to_list(&sector_list.list, &expanded_area);
		}
	}

	deinit_statemap(traversed_points);
	return sector_list;
}

void init_sector_list_vbo(OldSectorList* const sector_list) {
	List* const list = &sector_list -> list;

	buffer_size_t total_bytes = 0, total_components = 0;

	for (buffer_size_t i = 0; i < list -> length; i++) {
		const byte height = ((OldSector*) ptr_to_list_index(list, i)) -> height;
		total_bytes += (height == 0) ? bytes_per_face_11 : bytes_per_mesh;
	}

	mesh_type_t* const vertices = malloc(total_bytes);

	for (buffer_size_t i = 0; i < list -> length; i++) {
		const OldSector sector = value_at_list_index(list, i, OldSector);
		const mesh_type_t origin[3] = {sector.origin[0], sector.height, sector.origin[1]};

		if (sector.height == 0) { // Flat sector
			create_height_zero_mesh(origin, sector.size, vertices + total_components);
			sector_list -> num_vertices += vertices_per_triangle * triangles_per_face;
			total_components += vars_per_face;
		}
		else {
			const mesh_type_t size[3] = {sector.size[0], sector.height, sector.size[1]};
			create_sector_mesh(origin, size, vertices + total_components);
			sector_list -> num_vertices += vertices_per_triangle * triangles_per_mesh;
			total_components += vars_per_mesh;
		}
	}

	glGenBuffers(1, &sector_list -> vbo);
	glBindBuffer(GL_ARRAY_BUFFER, sector_list -> vbo);
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) total_bytes, vertices, GL_STATIC_DRAW);
}

StateGL configurable_demo_12_init(byte* const heightmap, const byte map_width, const byte map_height) {
	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0};

	OldSectorList sector_list = generate_sectors_from_heightmap(heightmap, map_width, map_height);
	init_sector_list_vbo(&sector_list);
	bind_sector_mesh_to_vao();

	OldSectorList* const sector_list_on_heap = malloc(sizeof(OldSectorList));
	*sector_list_on_heap = sector_list;
	sgl.any_data = sector_list_on_heap; // any_data stores sector meshes, and freed in demo_12_deinit

	sgl.shader_program = init_shader_program(sector_lighting_vertex_shader, sector_lighting_fragment_shader);
	glUseProgram(sgl.shader_program);
	enable_all_culling();
	glClearColor(0.89f, 0.855f, 0.788f, 0.0f); // Bone

	return sgl;	
}

StateGL demo_12_palace_init(void) {
	StateGL sgl = configurable_demo_12_init((byte*) palace_heightmap, palace_width, palace_height);

	sgl.num_textures = 1; // 14
	sgl.textures = init_plain_textures(sgl.num_textures,
		"../../../../assets/walls/pyramid_bricks_3.bmp", TexRepeating);
		/*
		"../../../../assets/walls/pyramid_bricks_4.bmp", TexRepeating,
		"../../../../assets/walls/hieroglyph.bmp", TexRepeating,
		"../../../../assets/walls/mesa.bmp", TexRepeating,
		"../../../../assets/walls/dune.bmp", TexRepeating,
		"../../../../assets/walls/cobblestone.bmp", TexRepeating,
		"../../../../assets/walls/hieroglyph.bmp", TexRepeating,
		"../../../../assets/walls/saqqara.bmp", TexRepeating,
		"../../../../assets/walls/trinity.bmp", TexRepeating,
		"../../../../assets/walls/cross_blue.bmp", TexRepeating,
		"../../../../assets/walls/dirt.bmp", TexRepeating,
		"../../../../assets/walls/dial.bmp", TexRepeating,
		"../../../../assets/walls/desert_snake.bmp", TexRepeating,
		"../../../../assets/walls/greece.bmp", TexRepeating,
		"../../../../assets/walls/arthouse_bricks.bmp", TexRepeating);
		*/

	return sgl;
}

StateGL demo_12_tpt_init(void) {
	StateGL sgl = configurable_demo_12_init((byte*) tpt_heightmap, tpt_width, tpt_height);
	sgl.num_textures = 1;
	sgl.textures = init_plain_textures(sgl.num_textures, "../../../../assets/walls/pyramid_bricks_2.bmp", TexRepeating);
	return sgl;
}

StateGL demo_12_pyramid_init(void) {
	StateGL sgl = configurable_demo_12_init((byte*) pyramid_heightmap, pyramid_width, pyramid_height);
	sgl.num_textures = 1;
	sgl.textures = init_plain_textures(sgl.num_textures, "../../../../assets/walls/greece.bmp", TexRepeating);
	return sgl;
}

StateGL demo_12_maze_init(void) {
	StateGL sgl = configurable_demo_12_init((byte*) maze_heightmap, maze_width, maze_height);
	sgl.num_textures = 1; // ivy
	sgl.textures = init_plain_textures(sgl.num_textures, "../../../../assets/walls/ivy.bmp", TexRepeating);
	return sgl;
}

void demo_12_drawer(const StateGL* const sgl) {
	static Camera camera;
	static PhysicsObject physics_obj = {.heightmap = (byte*) pyramid_heightmap, .map_size = {pyramid_width, pyramid_height}};
	static GLint camera_pos_world_space_id, model_view_projection_id;
	static bool first_call = true;

	const GLuint shader = sgl -> shader_program;

	if (first_call) {
		init_camera(&camera, (vec3) {1.5f, 1.0f, 1.5f});
		INIT_UNIFORM(camera_pos_world_space, shader);
		INIT_UNIFORM(model_view_projection, shader);
		first_call = false;
	}

	update_camera(&camera, get_next_event(), &physics_obj);

	UPDATE_UNIFORM(camera_pos_world_space, 3fv, 1, camera.pos);
	UPDATE_UNIFORM(model_view_projection, Matrix4fv, 1, GL_FALSE, &camera.model_view_projection[0][0]);

	use_texture(sgl -> textures[0], shader, "texture_sampler", TexPlain, 0);

	const OldSectorList* const sector_list = sgl -> any_data;
	glDrawArrays(GL_TRIANGLES, 0, sector_list -> num_vertices);
}

void demo_12_deinit(const StateGL* const sgl) {
	const OldSectorList* const sector_list = sgl -> any_data;
	deinit_sector_list(sector_list); // This frees the stored sector meshes + their vbo
	free(sgl -> any_data); // This frees the sector list struct on the heap

	deinit_demo_vars(sgl);
}

#ifdef DEMO_12
int main(void) {
	make_application(demo_12_drawer, demo_12_pyramid_init, demo_12_deinit);
}
#endif
