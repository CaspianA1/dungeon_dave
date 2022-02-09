/*
- Shadow maps for this demo - no shadow volumes, after some thinking.
- Start with making a plain shadow map: https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
- After that, move onto an omnidirectional shadow map: https://learnopengl.com/Advanced-Lighting/Shadows/Point-Shadows
- First, get depth buffer rendering working, and then make shadows. Done.
- Next, test more elaborate objects to be shadowed. Done.
- Next, use sampler shadows with GL_TEXTURE_COMPARE_MODE for no branch in the fragment shader.
- Next, add point lights.
*/

#include "../utils.c"
#include "../texture.c"
#include "../camera.c"

typedef struct { // `obj` here just means the objects in the scene
	const GLuint obj_shader, depth_map_shader, depth_map_texture;
	GLuint obj_vbo, depth_map_framebuffer;

	buffer_size_t num_obj_vertices;
	const buffer_size_t shadow_size[2];
} SceneState;

// TODO: make a shadows.c later, and put these shaders in shaders.c
const GLchar *const demo_23_obj_vertex_shader =
	"#version 330 core\n"

	"layout(location = 0) in vec3 vertex_pos_world_space;\n"

	"out vec4 shadow_coord;\n"

	"uniform mat4 light_bias_model_view_projection, obj_model_view_projection;\n"

	"void main(void) {\n"
		"vec4 vertex_pos_world_space_4D = vec4(vertex_pos_world_space, 1.0f);\n"
		"gl_Position = obj_model_view_projection * vertex_pos_world_space_4D;\n"
		"shadow_coord = light_bias_model_view_projection * vertex_pos_world_space_4D;\n"
	"}\n",

*const demo_23_obj_fragment_shader =
	"#version 330 core\n"

	"in vec4 shadow_coord;\n"

	"out vec3 color;\n"

	"uniform sampler2D depth_map_sampler;\n"

	"const float bias = 0.005f;\n"

	"void main(void) {\n"
		"float visibility = 1.0f;\n"

		"if (textureProj(depth_map_sampler, shadow_coord.xyw).z < bias / shadow_coord.w)\n"
			"visibility = 0.5f;\n"

		"color = visibility * vec3(0.2f, 0.5f, 0.7f);\n"
	"}\n",

*const demo_23_depth_map_vertex_shader =
	"#version 330 core\n"

	"layout(location = 0) in vec3 obj_vertex_pos_world_space;\n"

	"uniform mat4 light_model_view_projection;\n"

	"void main(void) {\n"
		"gl_Position = light_model_view_projection * vec4(obj_vertex_pos_world_space, 1.0f);\n"
	"}\n",

*const demo_23_depth_map_fragment_shader =
	"#version 330 core\n"

	"void main(void) {}\n";

GLuint init_demo_23_obj_vbo(buffer_size_t* const num_obj_vertices) {
	const GLint num_components_per_vertex = 3;
	const GLfloat plane_size[2] = {12.0f, 15.0f}; // X and Z

	#define OBJ_OFFSET(x, y, z) (x) + plane_size[0] * 0.5f, (y) + 2.0f, (z) + plane_size[1] * 0.5f

	const GLfloat vertices[] = {
		// Bottom left of flat plane
		plane_size[0], 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f,
		plane_size[0], 0.0f, plane_size[1],

		// Top right of flat plane
		0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, plane_size[1],
		plane_size[0], 0.0f, plane_size[1],

		// Little pyramid
		OBJ_OFFSET(-0.5f, 0.0f, -0.5f),
		OBJ_OFFSET(0.5f, 0.0f, -0.5f),
		OBJ_OFFSET(0.0f, 1.0f, 0.0f),

		OBJ_OFFSET(-0.5f, 0.0f, -0.5f),
		OBJ_OFFSET(-0.5f, 0.0f, 0.5f),
		OBJ_OFFSET(0.0f, 1.0f, 0.0f),

		OBJ_OFFSET(0.5f, 0.0f, 0.5f),
		OBJ_OFFSET(-0.5f, 0.0f, 0.5f),
		OBJ_OFFSET(0.0f, 1.0f, 0.0f),

		OBJ_OFFSET(0.5f, 0.0f, 0.5f),
		OBJ_OFFSET(0.5f, 0.0f, -0.5f),
		OBJ_OFFSET(0.0f, 1.0f, 0.0f),

		0.0f, 0.0f, plane_size[1] * 0.1f,
		plane_size[0], 0.0f, plane_size[1],
		plane_size[0] * 0.5f, 4.0f, plane_size[1]
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
		.obj_shader = init_shader_program(demo_23_obj_vertex_shader, demo_23_obj_fragment_shader),
		.depth_map_shader = init_shader_program(demo_23_depth_map_vertex_shader, demo_23_depth_map_fragment_shader),
		.depth_map_texture = preinit_texture(TexPlain, TexNonRepeating),
		.shadow_size = {128, 128}
	};

	scene_state.obj_vbo = init_demo_23_obj_vbo(&scene_state.num_obj_vertices);

	//////////

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		scene_state.shadow_size[0], scene_state.shadow_size[1],
		0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

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

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	return sgl;
}

void demo_23_drawer(const StateGL* const sgl) {
	glClearColor(0.5f, 0.0f, 0.0f, 0.0f);

	const SceneState scene_state = *((SceneState*) sgl -> any_data);

	static Camera camera;

	static GLint
		obj_model_view_projection_id, light_bias_model_view_projection_id, light_model_view_projection_id;

	static byte first_call = 1;

	if (first_call) {
		INIT_UNIFORM(obj_model_view_projection, scene_state.obj_shader);
		INIT_UNIFORM(light_bias_model_view_projection, scene_state.obj_shader);
		INIT_UNIFORM(light_model_view_projection, scene_state.depth_map_shader);
		init_camera(&camera, (vec3) {0.0f, 1.0f, 0.0f});
		first_call = 0;
	}

	////////// Math

	const Event event = get_next_event();
	update_camera(&camera, event, NULL);

	// TODO: set these up properly in camera.c later
	mat4 light_view, light_projection, light_view_projection, light_model_view_projection;

	get_view_matrix(
		(vec3) {4.460225f, 1.647446f, 2.147844f},
		(vec3) {0.853554f, -0.382683f, 0.353553f},
		(vec3) {-0.382683f, 0.0f, 0.923880f},
		light_view
	);

	glm_perspective(constants.camera.init.fov, (GLfloat) event.screen_size[0] / event.screen_size[1],
		constants.camera.clip_dists.near, constants.camera.clip_dists.far, light_projection);

	glm_mul(light_projection, light_view, light_view_projection);
	glm_mul(light_view_projection, GLM_MAT4_IDENTITY, light_model_view_projection);

	const mat4 bias_matrix = {
		{0.5f, 0.0f, 0.0f, 0.0f},
		{0.0f, 0.5f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.5f, 0.0f},
		{0.5f, 0.5f, 0.5f, 1.0f}
	};

	mat4 light_bias_model_view_projection;
	glm_mul((vec4*) light_model_view_projection, (vec4*) bias_matrix, light_bias_model_view_projection);

	////////// Rendering to depth map from light position

	glUseProgram(scene_state.depth_map_shader);
	UPDATE_UNIFORM(light_model_view_projection, Matrix4fv, 1, GL_FALSE, &light_model_view_projection[0][0]);

	glViewport(0, 0, scene_state.shadow_size[0], scene_state.shadow_size[1]);
	glBindFramebuffer(GL_FRAMEBUFFER, scene_state.depth_map_framebuffer); // No need to clear color + depth buffer
	glDrawArrays(GL_TRIANGLES, 0, scene_state.num_obj_vertices); // This line renders the scene
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	////////// Rendering the scene as normal

	glUseProgram(scene_state.obj_shader);
	UPDATE_UNIFORM(obj_model_view_projection, Matrix4fv, 1, GL_FALSE, &camera.model_view_projection[0][0]);
	UPDATE_UNIFORM(light_bias_model_view_projection, Matrix4fv, 1, GL_FALSE, &light_bias_model_view_projection[0][0]);

	use_texture(scene_state.depth_map_texture, scene_state.obj_shader, "depth_map_sampler", TexPlain, SHADOW_MAP_TEXTURE_UNIT);

	glViewport(0, 0, event.screen_size[0], event.screen_size[1]);
	glClear(GL_DEPTH_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLES, 0, scene_state.num_obj_vertices);
}

void demo_23_deinit(const StateGL* const sgl) {
	const SceneState scene_state = *((SceneState*) sgl -> any_data);

	glDeleteProgram(scene_state.obj_shader);
	glDeleteProgram(scene_state.depth_map_shader);
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
