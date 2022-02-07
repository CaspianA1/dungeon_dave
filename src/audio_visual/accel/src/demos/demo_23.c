/*
- Shadow maps for this demo - no shadow volumes, after some thinking.
- Start with making a plain shadow map: https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
- After that, move onto an omnidirectional shadow map: https://learnopengl.com/Advanced-Lighting/Shadows/Point-Shadows
*/

#include "../utils.c"
#include "../texture.c"
#include "../camera.c"

typedef struct { // `obj` here just means the objects in the scene
	const GLuint obj_shader, shadow_shader, depth_map_texture;
	GLuint obj_vbo, depth_map_framebuffer;

	buffer_size_t num_obj_vertices;
	const buffer_size_t shadow_size[2];
} SceneState;

const char *const demo_22_obj_vertex_shader =
	"#version 330 core\n"

	"layout(location = 0) in vec3 vertex_pos_world_space;\n"

	"out vec3 fragment_color;\n"

	"uniform mat4 obj_model_view_projection;\n"

	"void main(void) {\n"
		"fragment_color = vertex_pos_world_space / 8.0f;\n"
		"gl_Position = obj_model_view_projection * vec4(vertex_pos_world_space, 1.0f);\n"
	"}\n",

*const demo_22_obj_fragment_shader =
	"#version 330 core\n"

	"in vec3 fragment_color;\n"
	"out vec3 color;\n"

	"void main(void) {\n"
		"color = fragment_color;\n"
	"}\n",

*const demo_22_depth_map_vertex_shader =
	"#version 330 core\n"

	"uniform mat4 obj_model, light_view_projection;\n"

	"void main(void) {\n"

	"}\n",

*const demo_22_depth_map_fragment_shader =
	"#version 330 core\n"

	"void main(void) {\n"

	"}\n";

GLuint init_demo_23_obj_vbo(buffer_size_t* const num_obj_vertices) {
	const GLint num_components_per_vertex = 3;
	const GLfloat plane_size[2] = {10.0f, 8.0f}; // X and Z

	#define OBJ_OFFSET(x, y, z) (x) + plane_size[0] * 0.5f, (y) + 2.0f, (z) + plane_size[1] * 0.5f

	const GLfloat vertices[] = {
		// Bottom left of flat plane
		0.0f, 0.0f, 0.0f,
		plane_size[0], 0.0f, 0.0f,
		plane_size[0], 0.0f, plane_size[1],

		// Top right of flat plane
		0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, plane_size[1],
		plane_size[0], 0.0f, plane_size[1],

		OBJ_OFFSET(-1, -1, 0),
		OBJ_OFFSET(0, 0, 0),
		OBJ_OFFSET(1, -1, 0)
	};

	#undef OBJ_OFFSET

	*num_obj_vertices = sizeof(vertices) / sizeof(vertices[0]) / num_components_per_vertex;

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, num_components_per_vertex, GL_FLOAT, GL_FALSE, 0, (void*) 0);

	return vbo;
}

StateGL demo_23_init(void) {
	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0, .num_textures = 0};

	SceneState scene_state = {
		.obj_shader = init_shader_program(demo_22_obj_vertex_shader, demo_22_obj_fragment_shader),
		.shadow_shader = init_shader_program(demo_22_depth_map_vertex_shader, demo_22_depth_map_fragment_shader),
		.depth_map_texture = preinit_texture(TexPlain, TexNonRepeating),
		.shadow_size = {1024, 1024}
	};

	scene_state.obj_vbo = init_demo_23_obj_vbo(&scene_state.num_obj_vertices);

	//////////

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		scene_state.shadow_size[0], scene_state.shadow_size[1],
		0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);

	//////////

	glGenFramebuffers(1, &scene_state.depth_map_framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, scene_state.depth_map_framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, TexPlain, scene_state.depth_map_texture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//////////

	sgl.any_data = malloc(sizeof(SceneState));
	memcpy(sgl.any_data, &scene_state, sizeof(SceneState));

	return sgl;
}

void demo_23_drawer(const StateGL* const sgl) {
	const SceneState scene_state = *((SceneState*) sgl -> any_data);

	static Camera camera;
	static GLint obj_model_view_projection_id;
	static byte first_call = 1;

	if (first_call) {
		INIT_UNIFORM(obj_model_view_projection, scene_state.obj_shader);
		init_camera(&camera, (vec3) {0.0f, 1.0f, 0.0f});
		first_call = 0;
	}

	update_camera(&camera, get_next_event(), NULL);
	UPDATE_UNIFORM(obj_model_view_projection, Matrix4fv, 1, GL_FALSE, &camera.model_view_projection[0][0]);

	////////// Math

	mat4 light_view, light_projection, light_view_projection; // TODO: set these up properly in Camera later

	get_view_matrix(
		(vec3) {7.137089f, 2.295277f, 2.930087f},
		(vec3) {-0.529456f, -0.786935, 0.316875f},
		(vec3) {-0.513544f, 0.0f, -0.858063f},
		light_view
	);

	glm_ortho(-10.0f, 10.0f, -10.0f, 10.0f, constants.camera.clip_dists.near, constants.camera.clip_dists.far, light_projection);
	glm_mul(light_projection, light_view, light_view_projection);

	////////// Rendering to depth map from light position

	glViewport(0, 0, scene_state.shadow_size[0], scene_state.shadow_size[1]);
	glBindFramebuffer(GL_FRAMEBUFFER, scene_state.depth_map_framebuffer);
	// Depth and color buffer cleared at this point

	//////////

	// Rendering as scene with shadow mapping, using depth map
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, WINDOW_W, WINDOW_H);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindTexture(TexPlain, scene_state.depth_map_texture);

	// Rendering scene here

	glUseProgram(scene_state.obj_shader);
	glDrawArrays(GL_TRIANGLES, 0, scene_state.num_obj_vertices);
}

void demo_23_deinit(const StateGL* const sgl) {
	const SceneState scene_state = *((SceneState*) sgl -> any_data);

	glDeleteProgram(scene_state.obj_shader);
	glDeleteProgram(scene_state.shadow_shader);
	glDeleteBuffers(1, &scene_state.obj_vbo);
	glDeleteFramebuffers(1, &scene_state.depth_map_framebuffer);
	glDeleteTextures(1, &scene_state.depth_map_texture);

	free(sgl -> any_data);

	deinit_demo_vars(sgl);
}

#ifdef DEMO_23
int main(void) {
	make_application(demo_23_drawer, demo_23_init, demo_23_deinit);
}
#endif
