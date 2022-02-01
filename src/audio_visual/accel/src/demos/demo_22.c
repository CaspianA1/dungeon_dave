/*
Generating shadow volumes
- On a per-sector basis
- For each face in sector, find if facing towards or away from light
- Take away-facing faces
- The vertices that connect the forward-facing to away-facing faces form a silhouette
- Take those vertices, and form a volume outline
- Light source -> outline is not part of the shadow volume
- Shadow volume starts at silhouette vertices

- Forming a cone
For each vertex in the silhouette,
	- Get the vector between the light and the vertex
	- Extend that vector until it reaches a map boundary (or to another sector cube?)

- Make two caps on each end, as some triangles
- Make a shadow volume as triangles for the volume sides as well

Rendering shadow volumes

Depth pass:
- Disable writes to the depth and color buffers

- Set the stencil operation to increment on depth pass (only count shadows in front of the object/sector)
- Use backface culling, and render the shadow volumes (only front faces rendered, and only objs in front counted)

- Set the stencil operation to decrement on depth pass
- Use front face culling, and render the shadow volumes (only back faces rendered, and only objects behind counted)

- After this is accomplished, all lit surfaces will correspond to a 0 in the stencil buffer,
	where the numbers of front and back surfaces of all shadow volumes between the eye and that surface are equal.

- After this, all lit surfaces will have a 0 value in the stencil buffer,
	where the number of front and back surfaces of shadow volumes between eye and surface are equal
- 0 values in stencil buffer are lit then, and >= 1 values are darkened

- If stencil buf val is postiive, in shadow (more front facing than back facing)
- Zero, no shadow (since if you look through a shadow and just see another shadow through it, no shadow seen,
	since shadows are only defined by the objects that they hit)

Depth fail:
- Disable writes to the depth and color buffers
- Frontface culling, incr on depth fail (count shadows behind obj), and render
- Backface culling, decr on depth fail (decr for shadows in front of obj (i.e., visible)), and render (front faces rendered)
- Then, stencil buffer vals that equal 0 are lit

Optimization:
- Only think about it if shadow volumes are a performance problem

Other notes:
- Volume edges should maybe be at infinity
- And point light should perhaps solely be directional, not positional
- Is there a necessary reason for the volume taking up infinite space?
- Also, for constructing another shadow volume, primitive restart will be needed, which also requires indexing
- Construct other meshes within the same vbo for the shadow volume; one for the start cap, and one for the end

- Note: an occluder mesh with more than just 1 triangle does not work yet
- Also, find size for this and below (below size is probably num_vertices_per_mesh + 1)

- For building an ending bounding cap:
	- Construct a new vector, with the working axes in the right component indices
	- For each failed collision axis, put the minimum or maximum value for that axis in its index
*/

#include "../utils.c"
#include "../camera.c"
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

const GLchar *const demo_22_obj_vertex_shader =
	"#version 330 core\n"

	"layout(location = 0) in vec3 vertex_pos_world_space;\n"
	"layout(location = 1) in vec3 vertex_color;\n"

	"out vec3 fragment_color;\n"

	"uniform mat4 obj_model_view_projection;\n"

	"void main() {\n"
		"fragment_color = vertex_color;\n"
		"gl_Position = obj_model_view_projection * vec4(vertex_pos_world_space, 1.0f);\n"
	"}\n",

*const demo_22_obj_fragment_shader =
	"#version 330 core\n"

	"in vec3 fragment_color;\n"
	"out vec3 color;\n"

	"void main() {\n"
		"color = fragment_color;\n"
	"}\n",

*const demo_22_shadow_volume_vertex_shader =
	"#version 330 core\n"

	"layout(location = 0) in vec3 vertex_pos_world_space;\n"

	"out vec3 fragment_pos_world_space;\n"

	"uniform mat4 shadow_volume_model_view_projection;\n"

	"void main() {\n"
		"fragment_pos_world_space = vertex_pos_world_space;\n"
		"gl_Position = shadow_volume_model_view_projection * vec4(vertex_pos_world_space, 1.0f);\n"
	"}\n",

*const demo_22_shadow_volume_fragment_shader =
	"#version 330 core\n"

	"in vec3 fragment_pos_world_space;\n"

	"out vec4 color;\n"

	"void main() {\n"
		"color = vec4(fragment_pos_world_space / 5.0f, 0.5f);\n"
	"}\n";

const GLint num_components_per_vertex = 6, num_components_per_position = 3, num_components_per_color = 3;

// (p = p0 + dir * v) => (p - p0 = dir * v) => ((p - p0) / dir = v)
GLfloat get_v_for_ray_to_plane(const byte axis, const GLfloat flat_plane_size, const vec3 origin_vertex, const vec3 ray_dir) {
	GLfloat dividend = -origin_vertex[axis];
	const GLfloat dir_component = ray_dir[axis];
	if (dir_component > 0.0f) dividend += flat_plane_size;
	return dividend / dir_component;
}

GLfloat floats_eq(const GLfloat a, const GLfloat b) {
	return fabsf(b - a) < constants.almost_zero;
}

GLfloat vec3_eq(const vec3 a, const vec3 b) {
	// return floats_eq(a[0], b[0]) && floats_eq(a[1], b[1]) && floats_eq(a[2], b[2]);
	return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
}

// If the plane hit failed, returns the failing axis; otherwise, returns -1
signed_byte get_plane_hit_for_axis(const byte axis, const GLfloat flat_plane_size,
	const vec3 origin_vertex, const vec3 ray_dir, vec3 plane_hit) {

	const GLfloat v = get_v_for_ray_to_plane(axis, flat_plane_size, origin_vertex, ray_dir);
	const vec3 ray_v = {v, v, v};

	glm_vec3_copy((GLfloat*) origin_vertex, plane_hit);
	glm_vec3_muladd((GLfloat*) ray_dir, (GLfloat*) ray_v, plane_hit);

	for (signed_byte hit_axis = 0; hit_axis < 3; hit_axis++) {
		GLfloat hit_component = plane_hit[hit_axis];

		/* Sometimes, due to precision errors, values can be slightly less than 0, leading
		to a component being incorrectly recognized as out of bounds; so this adjusts that */
		if (floats_eq(hit_component, 0.0f)) hit_component = 0.0f;
		else if (floats_eq(hit_component, flat_plane_size)) hit_component = flat_plane_size;

		if (hit_component < 0.0f || hit_component > flat_plane_size)
			return hit_axis;

		plane_hit[hit_axis] = hit_component;
	}
	return -1;
}

void init_shadow_volume_buffers(ShadowVolumeContext* const context, const GLfloat flat_plane_size,
	const vec3 light_source_pos, const buffer_size_t num_components_per_whole_vertex,
	const buffer_size_t num_vertices_per_occluder_mesh, const GLfloat* const vertex_start) {

	List
		volume_vertices = init_list(1, vec3),
		volume_indices = init_list(1, buffer_size_t);

	push_ptr_to_list(&volume_vertices, light_source_pos);

	const buffer_size_t zero = 0;
	push_ptr_to_list(&volume_indices, &zero); // Pushing index of start of triangle fan

	for (buffer_size_t vertex_index = 0; vertex_index < num_vertices_per_occluder_mesh; vertex_index++) {
		const GLfloat* const origin_vertex = vertex_start + vertex_index * num_components_per_whole_vertex;

		vec3 ray_dir, plane_hit;
		glm_vec3_sub((GLfloat*) origin_vertex, (GLfloat*) light_source_pos, ray_dir);
		// glm_vec3_normalize(ray_dir); // Needed?

		////////// This part finds the intersection between the silhouette ray and the closest plane within bounds

		signed_byte colliding_axis = -1;
		for (signed_byte test_axis = 0; test_axis < 3; test_axis++) {
			const signed_byte failing_axis = get_plane_hit_for_axis(test_axis, flat_plane_size, origin_vertex, ray_dir, plane_hit);
			if (failing_axis == -1) {
				colliding_axis = test_axis;
				break;
			}
		}

		if (colliding_axis == -1) fail("Unable to clip shadow volume mesh inside bounding area", CreateMesh);

		////////// This part finds if the colliding vertex was a repeat; and if so, adds the old index to the index buffer

		byte repeated_colliding_vertex = 0;
		for (buffer_size_t prev_colliding_index = 0; prev_colliding_index < volume_vertices.length; prev_colliding_index++) {
			const GLfloat* const prev_colliding_vertex = (GLfloat*) volume_vertices.data + prev_colliding_index * 3;
			if (vec3_eq(prev_colliding_vertex, plane_hit)) {
				push_ptr_to_list(&volume_indices, &prev_colliding_index);
				repeated_colliding_vertex = 1;
				break;
			}
		}

		//////////

		if (!repeated_colliding_vertex) {
			push_ptr_to_list(&volume_vertices, plane_hit);
			const buffer_size_t incr_vertex_index = vertex_index + 1; // b/c first index value is the light position
			push_ptr_to_list(&volume_indices, &incr_vertex_index);
		}
	}

	context -> shadow_volume.num_indices = volume_indices.length;

	glGenBuffers(1, &context -> shadow_volume.vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, context -> shadow_volume.vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, volume_vertices.length * sizeof(vec3), volume_vertices.data, GL_STATIC_DRAW);

	glGenBuffers(1, &context -> shadow_volume.index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, context -> shadow_volume.index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, volume_indices.length * sizeof(buffer_size_t), volume_indices.data, GL_STATIC_DRAW);

	deinit_list(volume_indices);
	deinit_list(volume_vertices);
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
		GL_FLOAT, GL_FALSE, num_components_per_vertex * sizeof(GLfloat), (void*) 0);

	glVertexAttribPointer(1, num_components_per_color, GL_FLOAT,
		GL_FALSE, num_components_per_vertex * sizeof(GLfloat),
		(void*) (num_components_per_position * sizeof(GLfloat)));

	glDrawArrays(GL_TRIANGLES, 0, context.obj.num_vertices);
	glDisableVertexAttribArray(1);

	//////////

	glUseProgram(context.shadow_volume.shader);
	UPDATE_UNIFORM(shadow_volume_model_view_projection, Matrix4fv, 1, GL_FALSE, &model_view_projection[0][0]);
	glBindBuffer(GL_ARRAY_BUFFER, context.shadow_volume.vertex_buffer);

	glVertexAttribPointer(0, num_components_per_position, GL_FLOAT, GL_FALSE, 0, (void*) 0);

	glEnable(GL_BLEND);
	glDrawElements(GL_TRIANGLE_FAN, 5, BUFFER_SIZE_TYPENAME, (void*) 0);
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

		PYRAMID_OFFSET(-0.5f, 0.0f, -0.5f), occluder_color[0], occluder_color[1], occluder_color[2],

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
		flat_plane_size, light_source_pos, num_components_per_vertex,
		12, &vertices[num_components_per_vertex * 6] // For num_vertices_per_mesh, 12 at most
	);

	return context;
}

StateGL demo_22_init(void) {
	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0, .num_textures = 0};

	//  {2.33f, 2.13f, 5.03f} goes on 1 axis; {2.3f, 0.486f, 2.6f} is more problematic. Normal start: {2.425301, 0.111759, 2.026766}
	ShadowVolumeContext context = init_shadow_volume_context((vec3) {5.0f, 0.486f, 0.0f});
	sgl.any_data = malloc(sizeof(ShadowVolumeContext));
	memcpy(sgl.any_data, &context, sizeof(ShadowVolumeContext));

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnableVertexAttribArray(0);

	return sgl;
}

void demo_22_drawer(const StateGL* const sgl) {
	const ShadowVolumeContext context = *((ShadowVolumeContext*) sgl -> any_data);

	/*
	static vec3 light_pos = {5.0f, 0.486f, 0.0f};
	const GLfloat step = 0.05f;
	if (keys[SDL_SCANCODE_I]) light_pos[0] += step;
	if (keys[SDL_SCANCODE_K]) light_pos[0] -= step;
	if (keys[SDL_SCANCODE_SEMICOLON]) light_pos[1] += step;
	if (keys[SDL_SCANCODE_APOSTROPHE]) light_pos[1] -= step;
	if (keys[SDL_SCANCODE_J]) light_pos[2] += step;
	if (keys[SDL_SCANCODE_L]) light_pos[2] -= step;

	for (byte i = 0; i < 3; i++) {
		GLfloat pos_component = light_pos[i];
		if (pos_component < 0.0f) pos_component = 0.0f;
		else if (pos_component > 5.0f) pos_component = 5.0f;
		light_pos[i] = pos_component;
	}

	DEBUG_VEC3(light_pos);
	context = init_shadow_volume_context(light_pos);
	*/

	static Camera camera;
	static byte first_call = 1;

	if (first_call) {
		init_camera(&camera, (vec3) {1.0f, 1.0f, 1.0f});
		first_call = 0;
	}
	glClearColor(0.89f, 0.855f, 0.788f, 0.0f); // Bone

	update_camera(&camera, get_next_event(), NULL);
	draw_shadow_volume_context(context, camera.model_view_projection);
}

void demo_22_deinit(const StateGL* const sgl) {
	const ShadowVolumeContext context = *((ShadowVolumeContext*) sgl -> any_data);
	glDeleteBuffers(1, &context.obj.vertex_buffer);
	glDeleteBuffers(1, &context.shadow_volume.vertex_buffer);
	glDeleteBuffers(1, &context.shadow_volume.index_buffer);

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
