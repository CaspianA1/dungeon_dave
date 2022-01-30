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
*/

#include "../utils.c"
#include "../camera.c"
#include "../list.c"

typedef struct {
	GLuint obj_buffer, shadow_volume_buffer, obj_shader, shadow_volume_shader;
	GLsizeiptr num_obj_vertices, num_volume_vertices;
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
		"color = vec4(fragment_pos_world_space / 20.0f, 0.8f);\n"
	"}\n";

const GLint num_components_per_vertex = 6, num_components_per_position = 3, num_components_per_color = 3;

GLuint init_shadow_volume_buffer(GLsizeiptr* const num_volume_vertices, const GLfloat flat_plane_size,
	const vec3 light_source_pos, const buffer_size_t num_components_per_whole_vertex,
	const buffer_size_t num_vertices_per_mesh, const GLfloat* const vertex_start) {

	List cone_vertices = init_list(1, vec3);
	push_ptr_to_list(&cone_vertices, light_source_pos);

	/* To begin with, form a cone between the camera and the vertices

	For each vertex, form a connection
	Store those in a list
	Then, build triangles out of the connections

	Later on, reverse it

	And at some point, discard forward-facing vertices

	All vertices must actually not share the same plane; they can, but it's optional */

	const buffer_size_t total_num_components = num_components_per_whole_vertex * num_vertices_per_mesh;
	for (buffer_size_t i = 0; i < total_num_components; i += num_components_per_whole_vertex) {
		const GLfloat* const vertex_pos = vertex_start + i;

		vec3 direction;
		glm_vec3_sub((GLfloat*) vertex_pos, (GLfloat*) light_source_pos, direction);
		glm_vec3_normalize(direction);

		/* - Intersecting plane will be flat
		- And for this plane, y = 0

		Ray:
		p = p0 + dir * v
		Solve for v when p.y == 0

		p.xyz = p0.xyz + dir.xyz * v

		p.y = p0.y + dir.y * v
		0 = p0.y + dir.y * v
		-p0.y = dir.y * v
		-p0.y / dir.y = v

		- Find closest plane intersection for all 3 vertices; smallest resulting distance for component
		- And for non-intersecting planes, detect if any intersection happens at all
		*/

		vec3 v;

		for (byte i = 0; i < 3; i++) {
			GLfloat dividend = -vertex_pos[1];
			const GLfloat dir_y = direction[1];
			if (dir_y > 0.0f) dividend += flat_plane_size;
			v[i] = dividend / dir_y;
			// Find the shortest distance to a plane with this direction, given a vertes
		}

		vec3 plane_endpoint;
		glm_vec3_copy((GLfloat*) vertex_pos, plane_endpoint);
		glm_vec3_muladd(direction, v, plane_endpoint);

		push_ptr_to_list(&cone_vertices, plane_endpoint);
	}

	*num_volume_vertices = cone_vertices.length;

	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, cone_vertices.length * sizeof(vec3), cone_vertices.data, GL_STATIC_DRAW);

	deinit_list(cone_vertices);
	return buffer;
}

void draw_shadow_volume_context(const ShadowVolumeContext context, mat4 model_view_projection) {
	static GLint obj_model_view_projection_id, shadow_volume_model_view_projection_id;
	static byte first_call = 1;

	if (first_call) {
		INIT_UNIFORM(obj_model_view_projection, context.obj_shader);
		INIT_UNIFORM(shadow_volume_model_view_projection, context.shadow_volume_shader);
		first_call = 0;
	}

	glEnableVertexAttribArray(0);

	//////////

	glUseProgram(context.obj_shader);
	UPDATE_UNIFORM(obj_model_view_projection, Matrix4fv, 1, GL_FALSE, &model_view_projection[0][0]);
	glBindBuffer(GL_ARRAY_BUFFER, context.obj_buffer);
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(0, num_components_per_position,
		GL_FLOAT, GL_FALSE, num_components_per_vertex * sizeof(GLfloat), (void*) 0);

	glVertexAttribPointer(1, num_components_per_color, GL_FLOAT,
		GL_FALSE, num_components_per_vertex * sizeof(GLfloat),
		(void*) (num_components_per_position * sizeof(GLfloat)));

	glDrawArrays(GL_TRIANGLES, 0, context.num_obj_vertices);
	glDisableVertexAttribArray(1);

	//////////

	glEnable(GL_BLEND);

	glUseProgram(context.shadow_volume_shader);
	UPDATE_UNIFORM(shadow_volume_model_view_projection, Matrix4fv, 1, GL_FALSE, &model_view_projection[0][0]);
	glBindBuffer(GL_ARRAY_BUFFER, context.shadow_volume_buffer);
	glVertexAttribPointer(0, num_components_per_position, GL_FLOAT, GL_FALSE, 0, (void*) 0);
	glDrawArrays(GL_TRIANGLE_FAN, 0, context.num_volume_vertices);

	glDisable(GL_BLEND);

	//////////

	glDisableVertexAttribArray(0);
}

ShadowVolumeContext init_shadow_volume_context(const vec3 light_source_pos) {
	ShadowVolumeContext context = {
		.obj_shader = init_shader_program(demo_22_obj_vertex_shader, demo_22_obj_fragment_shader),
		.shadow_volume_shader = init_shader_program(demo_22_shadow_volume_vertex_shader, demo_22_shadow_volume_fragment_shader)
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

	context.num_obj_vertices = sizeof(vertices) / sizeof(vertices[0]) / num_components_per_vertex;

	glGenBuffers(1, &context.obj_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, context.obj_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	context.shadow_volume_buffer = init_shadow_volume_buffer(&context.num_volume_vertices,
		flat_plane_size, light_source_pos, num_components_per_vertex,
		4, &vertices[num_components_per_vertex * 6]
	);

	return context;
}

StateGL demo_22_init(void) {
	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0, .num_textures = 0};

	ShadowVolumeContext context = init_shadow_volume_context((vec3) {2.3f, 0.486f, 2.6f});
	sgl.any_data = malloc(sizeof(ShadowVolumeContext));
	memcpy(sgl.any_data, &context, sizeof(ShadowVolumeContext));

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	return sgl;
}

void demo_22_drawer(const StateGL* const sgl) {
	const ShadowVolumeContext context = *((ShadowVolumeContext*) sgl -> any_data);

	static Camera camera;
	static byte first_call = 1;

	if (first_call) {
		init_camera(&camera, (vec3) {1.0f, 1.0f, 1.0f});
		first_call = 0;
	}

	update_camera(&camera, get_next_event(), NULL);
	draw_shadow_volume_context(context, camera.model_view_projection);
}

void demo_22_deinit(const StateGL* const sgl) {
	ShadowVolumeContext context = *((ShadowVolumeContext*) sgl -> any_data);
	glDeleteBuffers(1, &context.obj_buffer);
	glDeleteBuffers(1, &context.shadow_volume_buffer);
	glDeleteProgram(context.obj_shader);
	glDeleteProgram(context.shadow_volume_shader);

	free(sgl -> any_data);
	deinit_demo_vars(sgl);
}

#ifdef DEMO_22
int main(void) {
	make_application(demo_22_drawer, demo_22_init, demo_22_deinit);
}
#endif
