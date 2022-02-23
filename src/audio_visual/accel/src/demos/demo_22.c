/* Generating shadow volumes
- On a per-sector basis
- For each face in sector, find if facing towards or away from light
- Take away-facing faces
- The vertices that connect the forward-facing to away-facing faces form a silhouette
- Take those vertices, and form a volume outline
- Shadow volume starts at silhouette vertices

Rendering via depth fail:
- Disable writes to the depth and color buffers
- Frontface culling, incr on depth fail (count shadows behind obj), and render
- Backface culling, decr on depth fail (decr for shadows in front of obj (i.e., visible)), and render (front faces rendered)
- Then, stencil buffer vals that equal 0 are lit

- Building a bounded end cap may double the number of vertices
- For that, extend the v to get all of the planes outside
- Or perhaps infinity, if the precision troubles work out

- One shadow volume per sector or per face?
	Per face may be necessary for more accuracy, so start with per face
	(note: per sector overall face, not small faces used to make meshes)
	Start with emitting one volume for one input face, with no sector input

For later:
- See if I can optimize the mesh with GL_TRIANGLE_STRIP/GL_TRIANGLE_FAN with primitive restart
Currently:
- For a cuboidal mesh, find its silhouette (border between forward-facing and back-facing faces?)
- Silhouette finding: dimension data -> simplified cube,
	and then test the 4 vert faces + top flat face (light from under possible)
	from that (test from side on axis-aligned plane)
- From that silhouette, build a 4-sided start cap
- Make an infinitely far away end-cap
- Build the 4 sides of the mesh */

#include "../utils.c"
#include "../camera.c"
#include "../event.c"
#include "../list.c"

typedef struct {
	struct {
		GLuint vertex_buffer, shader;
		buffer_size_t num_vertices;
	} obj;

	struct {
		GLuint vertex_buffer, index_buffer, shader;
		buffer_size_t num_indices;
	} shadow_volume;
} ShadowVolumeContext;

typedef enum {
	X, Y, Z, NoAxis
} Axis;

const GLchar *const demo_22_obj_vertex_shader =
	"#version 330 core\n"

	"layout(location = 0) in vec3 vertex_pos_world_space;\n"
	"layout(location = 1) in vec3 vertex_color;\n"

	"out vec3 fragment_color;\n"

	"uniform mat4 obj_model_view_projection;\n"

	"void main(void) {\n"
		"fragment_color = vertex_color;\n"
		"gl_Position = obj_model_view_projection * vec4(vertex_pos_world_space, 1.0f);\n"
	"}\n",

*const demo_22_obj_fragment_shader =
	"#version 330 core\n"

	"in vec3 fragment_color;\n"
	"out vec3 color;\n"

	"void main(void) {\n"
		"color = fragment_color;\n"
	"}\n",

*const demo_22_shadow_volume_vertex_shader =
	"#version 330 core\n"

	"layout(location = 0) in vec3 vertex_pos_world_space;\n"

	"out vec3 fragment_pos_world_space;\n"

	"uniform mat4 shadow_volume_model_view_projection;\n"

	"void main(void) {\n"
		"fragment_pos_world_space = vertex_pos_world_space;\n"
		"gl_Position = shadow_volume_model_view_projection * vec4(vertex_pos_world_space, 1.0f);\n"
	"}\n",

*const demo_22_shadow_volume_fragment_shader =
	"#version 330 core\n"

	"in vec3 fragment_pos_world_space;\n"

	"out vec4 color;\n"

	"void main(void) {\n"
		"color = vec4(fragment_pos_world_space / 5.0f, 0.5f);\n"
	"}\n";

const GLint num_components_per_vertex = 6, num_components_per_position = 3, num_components_per_color = 3;

void init_shadow_volume_buffers(ShadowVolumeContext* const context,
	const vec3 light_source_pos, const buffer_size_t num_components_per_whole_vertex,
	const buffer_size_t num_vertices_per_occluder_mesh, const GLfloat* const occluder_vertex_start) {

	List volume_vertices = init_list(1, vec3), volume_indices = init_list(1, buffer_size_t);

	////////// Generating start cap of shadow volume

	for (buffer_size_t vertex_index = 0; vertex_index < num_vertices_per_occluder_mesh; vertex_index++) {
		const GLfloat* const front_cap_vertex = occluder_vertex_start + vertex_index * num_components_per_whole_vertex;
		push_ptr_to_list(&volume_vertices, front_cap_vertex);
		push_ptr_to_list(&volume_indices, &vertex_index);
	}

	////////// Adding infinity-projected edges and end cap indices to cpu buffers

	const buffer_size_t projected_end_index_start = volume_indices.length;

	const GLfloat ray_length = 5.0f;

	for (buffer_size_t vertex_index = 0; vertex_index < num_vertices_per_occluder_mesh; vertex_index++) {
		const GLfloat* const front_cap_vertex = ptr_to_list_index(&volume_vertices, vertex_index);

		vec3 ray_dir, end_cap_vertex;

		glm_vec3_sub((GLfloat*) front_cap_vertex, (GLfloat*) light_source_pos, ray_dir); // Getting dir
		glm_vec3_normalize(ray_dir);

		glm_vec3_copy((GLfloat*) front_cap_vertex, end_cap_vertex); // p = p0
		glm_vec3_muladds(ray_dir, ray_length, end_cap_vertex); // p += length * dir

		push_ptr_to_list(&volume_vertices, end_cap_vertex);

		const buffer_size_t end_cap_index = projected_end_index_start + vertex_index;
		push_ptr_to_list(&volume_indices, &end_cap_index);
	}

	////////// Adding shadow volume edges to cpu index buffer

	push_array_to_list(&volume_indices, (buffer_size_t[6]) {0, 3, 4, 0, 1, 4}, 6);
	push_array_to_list(&volume_indices, (buffer_size_t[6]) {1, 4, 5, 1, 2, 5}, 6);
	push_array_to_list(&volume_indices, (buffer_size_t[6]) {0, 3, 5, 0, 2, 5}, 6);

	////////// Generating shadow volume gpu buffers

	context -> shadow_volume.num_indices = volume_indices.length;

	glGenBuffers(1, &context -> shadow_volume.vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, context -> shadow_volume.vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3[volume_vertices.length]), volume_vertices.data, GL_STATIC_DRAW);

	glGenBuffers(1, &context -> shadow_volume.index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, context -> shadow_volume.index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(buffer_size_t[volume_indices.length]), volume_indices.data, GL_STATIC_DRAW);

	////////// Deinitializing shadow volume cpu buffers

	deinit_list(volume_vertices);
	deinit_list(volume_indices);
}

void draw_shadow_volume_context(const ShadowVolumeContext context, mat4 model_view_projection) {
	static GLint obj_model_view_projection_id, shadow_volume_model_view_projection_id;
	static byte first_call = 1;

	if (first_call) {
		INIT_UNIFORM(obj_model_view_projection, context.obj.shader);
		INIT_UNIFORM(shadow_volume_model_view_projection, context.shadow_volume.shader);
		first_call = 0;
	}

	//////////

	glUseProgram(context.obj.shader);
	UPDATE_UNIFORM(obj_model_view_projection, Matrix4fv, 1, GL_FALSE, &model_view_projection[0][0]);
	glBindBuffer(GL_ARRAY_BUFFER, context.obj.vertex_buffer);

	glEnableVertexAttribArray(1);

	glVertexAttribPointer(0, num_components_per_position,
		GL_FLOAT, GL_FALSE, sizeof(GLfloat[num_components_per_vertex]), (void*) 0);

	glVertexAttribPointer(1, num_components_per_color, GL_FLOAT,
		GL_FALSE, sizeof(GLfloat[num_components_per_vertex]),
		(void*) sizeof(GLfloat[num_components_per_position]));

	if (!keys[SDL_SCANCODE_C]) glDrawArrays(GL_TRIANGLES, 0, context.obj.num_vertices);
	glDisableVertexAttribArray(1);

	//////////

	glUseProgram(context.shadow_volume.shader);
	UPDATE_UNIFORM(shadow_volume_model_view_projection, Matrix4fv, 1, GL_FALSE, &model_view_projection[0][0]);
	glBindBuffer(GL_ARRAY_BUFFER, context.shadow_volume.vertex_buffer);

	glVertexAttribPointer(0, num_components_per_position, GL_FLOAT, GL_FALSE, 0, (void*) 0);

	glEnable(GL_BLEND);
	glDrawElements(GL_TRIANGLES, context.shadow_volume.num_indices, BUFFER_SIZE_TYPENAME, (void*) 0);
	glDisable(GL_BLEND);
}

ShadowVolumeContext init_shadow_volume_context(const vec3 light_source_pos) {
	ShadowVolumeContext context = {
		.obj.shader = init_shader_program(demo_22_obj_vertex_shader, demo_22_obj_fragment_shader),
		.shadow_volume.shader = init_shader_program(demo_22_shadow_volume_vertex_shader, demo_22_shadow_volume_fragment_shader)
	};

	const GLfloat
		flat_plane_size = 5.0f,
		flat_plane_color[3] = {0.5f, 0.0f, 0.0f},
		occluder_color[3] = {0.3f, 0.5f, 0.8f};

	#define PYRAMID_OFFSET(x, y, z) 2.5f + (x), (y) + 1.0f, 2.5f + (z)

	const GLfloat vertices[] = {
		// Flat plane

		0.0f, 0.0f, 0.0f, flat_plane_color[0], flat_plane_color[1], flat_plane_color[2],
		flat_plane_size, 0.0f, flat_plane_size, flat_plane_color[0], flat_plane_color[1], flat_plane_color[2],
		0.0f, 0.0f, flat_plane_size, flat_plane_color[0], flat_plane_color[1], flat_plane_color[2],

		0.0f, 0.0f, 0.0f, flat_plane_color[0], flat_plane_color[1], flat_plane_color[2],
		flat_plane_size, 0.0f, flat_plane_size, flat_plane_color[0], flat_plane_color[1], flat_plane_color[2],
		flat_plane_size, 0.0f, 0.0f, flat_plane_color[0], flat_plane_color[1], flat_plane_color[2],

		// Occluder

		PYRAMID_OFFSET(-0.5f, 0.0f, -0.5f), occluder_color[0], occluder_color[1], occluder_color[2],
		PYRAMID_OFFSET(0.5f, 0.0f, -0.5f), occluder_color[0], occluder_color[1] - 0.3f, occluder_color[2],
		PYRAMID_OFFSET(0.0f, 1.0f, 0.0f), occluder_color[0], occluder_color[1], occluder_color[2],

		/*
		PYRAMID_OFFSET(-0.5f, 0.0f, -0.5f), occluder_color[0] + 0.3f, occluder_color[1], occluder_color[2],
		PYRAMID_OFFSET(-0.5f, 0.0f, 0.5f), occluder_color[0], occluder_color[1], occluder_color[2],
		PYRAMID_OFFSET(0.0f, 1.0f, 0.0f), occluder_color[0], occluder_color[1], occluder_color[2],
		
		PYRAMID_OFFSET(0.5f, 0.0f, 0.5f), occluder_color[0] + 0.3f, occluder_color[1], occluder_color[2],
		PYRAMID_OFFSET(-0.5f, 0.0f, 0.5f), occluder_color[0], occluder_color[1], occluder_color[2],
		PYRAMID_OFFSET(0.0f, 1.0f, 0.0f), occluder_color[0], occluder_color[1], occluder_color[2],

		PYRAMID_OFFSET(0.5f, 0.0f, 0.5f), occluder_color[0] + 0.3f, occluder_color[1], occluder_color[2],
		PYRAMID_OFFSET(0.5f, 0.0f, -0.5f), occluder_color[0], occluder_color[1], occluder_color[2],
		PYRAMID_OFFSET(0.0f, 1.0f, 0.0f), occluder_color[0], occluder_color[1], occluder_color[2],
		*/
	};

	#undef PYRAMID_OFFSET

	context.obj.num_vertices = sizeof(vertices) / sizeof(vertices[0]) / num_components_per_vertex;

	glGenBuffers(1, &context.obj.vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, context.obj.vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	init_shadow_volume_buffers(&context,
		light_source_pos, num_components_per_vertex,
		3, &vertices[num_components_per_vertex * 6] // For num_vertices_per_mesh, 12 at most
	);

	return context;
}

StateGL demo_22_init(void) {
	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0, .num_textures = 0};

	const ShadowVolumeContext context = init_shadow_volume_context((vec3) {4.0f, 3.05f, 0.0f});
	sgl.any_data = malloc(sizeof(ShadowVolumeContext));
	memcpy(sgl.any_data, &context, sizeof(ShadowVolumeContext));

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnableVertexAttribArray(0);
	glClearColor(0.89f, 0.855f, 0.788f, 0.0f); // Bone

	return sgl;
}

void demo_22_drawer(const StateGL* const sgl) {
	const ShadowVolumeContext context = *((ShadowVolumeContext*) sgl -> any_data);

	/*
	static vec3 light_pos = {4.0f, 3.05f, 0.0f};
	const GLfloat step = 0.05f;
	if (keys[SDL_SCANCODE_I]) light_pos[0] += step;
	if (keys[SDL_SCANCODE_K]) light_pos[0] -= step;
	if (keys[SDL_SCANCODE_SEMICOLON]) light_pos[1] += step;
	if (keys[SDL_SCANCODE_APOSTROPHE]) light_pos[1] -= step;
	if (keys[SDL_SCANCODE_J]) light_pos[2] += step;
	if (keys[SDL_SCANCODE_L]) light_pos[2] -= step;
	*/

	/*
	for (byte i = 0; i < 3; i++) {
		GLfloat pos_component = light_pos[i];
		if (pos_component < 0.0f) pos_component = 0.0f;
		else if (pos_component > 5.0f) pos_component = 5.0f;
		light_pos[i] = pos_component;
	}
	*/

	/*
	DEBUG_VEC3(light_pos);
	context = init_shadow_volume_context(light_pos);
	*/

	static Camera camera;
	static byte first_call = 1;

	if (first_call) {
		init_camera(&camera, (vec3) {4.0f, 3.05f, 0.0f});
		first_call = 0;
	}

	update_camera(&camera, get_next_event(), NULL);
	draw_shadow_volume_context(context, camera.model_view_projection);
}

void demo_22_deinit(const StateGL* const sgl) {
	const ShadowVolumeContext context = *((ShadowVolumeContext*) sgl -> any_data);

	const GLuint buffers[3] = {
		context.obj.vertex_buffer,
		context.shadow_volume.vertex_buffer,
		context.shadow_volume.index_buffer
	};

	glDeleteBuffers(3, buffers);

	glDeleteProgram(context.obj.shader);
	glDeleteProgram(context.shadow_volume.shader);

	free(sgl -> any_data);
	deinit_demo_vars(sgl);
}

#ifdef DEMO_22
int main(void) {
	make_application(demo_22_drawer, demo_22_init, demo_22_deinit);
}
#endif
