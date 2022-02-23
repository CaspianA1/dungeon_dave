#ifndef SHADOW_MAP_C
#define SHADOW_MAP_C

/*
- Overlapping shadows
- Popping lights on re-rendering (it seems to come from some lag in updating shadows)
- Some peter panning and shadow acne
- Get smooth shadows through variance shadow mapping
- A light source for the shadow map context that isn't based on the camera
- Shadows for billboards and the weapon
- Some values to the side are lit up when they shouldn't; perhaps try an orthographic matrix
- If not, make the light FOV an input to init_shadow_map_context
*/

#include "headers/shadow_map.h"

// TODO: to shaders.c
const GLchar *const depth_vertex_shader =
	"#version 330 core\n"

	"layout (location = 0) in vec3 vertex_pos_world_space;\n"

	"uniform mat4 light_model_view_projection;\n"

	"void main(void) {\n"
		"gl_Position = light_model_view_projection * vec4(vertex_pos_world_space, 1.0f);\n"
	"}\n",

*const depth_fragment_shader =
	"#version 330 core\n"

	"void main(void) {}\n";

ShadowMapContext init_shadow_map_context(
	const GLsizei shadow_width, const GLsizei shadow_height,
	const vec3 light_pos, const vec3 light_dir, vec3 light_up) {
	
	GLuint texture, framebuffer;

	glGenTextures(1, &texture);
	glBindTexture(TexPlain, texture);
	glTexParameteri(TexPlain, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(TexPlain, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(TexPlain, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(TexPlain, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(TexPlain, GL_TEXTURE_BORDER_COLOR, (GLfloat[4]) {1.0f, 1.0f, 1.0f, 1.0f});  

	glTexParameteri(TexPlain, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(TexPlain, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	glTexImage2D(TexPlain, 0, GL_DEPTH_COMPONENT, shadow_width, shadow_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, TexPlain, texture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		fail("make a framebuffer; framebuffer not complete", CreateFramebuffer);

	const GLuint shader = init_shader_program(depth_vertex_shader, depth_fragment_shader);

	return (ShadowMapContext) {
		.shader_context = {shader, .INIT_UNIFORM(light_model_view_projection, shader)},
		.depth_map = {texture, framebuffer},
		.shadow_size = {shadow_width, shadow_height},

		.light_context = {
			{light_pos[0], light_pos[1], light_pos[2]},
			{light_dir[0], light_dir[1], light_dir[2]},
			{light_up[0], light_up[1], light_up[2]},
			.model_view_projection = {0}
		}
	};
}

void deinit_shadow_map_context(const ShadowMapContext* const shadow_map_context) {
	deinit_texture(shadow_map_context -> depth_map.texture);
	glDeleteFramebuffers(1, &shadow_map_context -> depth_map.framebuffer);
	glDeleteProgram(shadow_map_context -> shader_context.depth_shader);
}

static void enable_rendering_to_shadow_map(ShadowMapContext* const shadow_map_context_ref) {
	ShadowMapContext shadow_map_context = *shadow_map_context_ref;

	const GLsizei
		shadow_width = shadow_map_context.shadow_size[0],
		shadow_height = shadow_map_context.shadow_size[1];
	
	//////////

	// These matrices are relative to the light
	mat4 view, projection;

	glm_look((GLfloat*) shadow_map_context.light_context.pos,
		(GLfloat*) shadow_map_context.light_context.dir,
		(GLfloat*) shadow_map_context.light_context.up, view);

	glm_perspective(constants.camera.init.fov, (GLfloat) shadow_width / shadow_height,
		constants.camera.clip_dists.near, constants.camera.clip_dists.far, projection);

	glm_mul(projection, view, shadow_map_context.light_context.model_view_projection);

	//////////

	glUseProgram(shadow_map_context.shader_context.depth_shader);

	UPDATE_UNIFORM(shadow_map_context.shader_context.light_model_view_projection,
		Matrix4fv, 1, GL_FALSE, (GLfloat*) shadow_map_context.light_context.model_view_projection);

	glViewport(0, 0, shadow_width, shadow_height);
	glBindFramebuffer(GL_FRAMEBUFFER, shadow_map_context.depth_map.framebuffer);
	glClear(GL_DEPTH_BUFFER_BIT); // Clear framebuffer
	glCullFace(GL_FRONT);

	memcpy(shadow_map_context_ref -> light_context.model_view_projection,
		shadow_map_context.light_context.model_view_projection, sizeof(mat4));
}

static void disable_rendering_to_shadow_map(const int screen_size[2]) {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, screen_size[0], screen_size[1]);
	glCullFace(GL_BACK);
}

void render_all_sectors_to_shadow_map(ShadowMapContext* const shadow_map_context,
	const BatchDrawContext* const sector_draw_context, const int screen_size[2]) {

	glBindBuffer(GL_ARRAY_BUFFER, sector_draw_context -> buffers.gpu);

	GLint bytes_for_vertices;
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bytes_for_vertices);
	const GLsizei total_num_vertices = bytes_for_vertices / bytes_per_face * vertices_per_face;

	glBufferSubData(GL_ARRAY_BUFFER, 0, bytes_for_vertices, sector_draw_context -> buffers.cpu.data);

	enable_rendering_to_shadow_map(shadow_map_context);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, MESH_COMPONENT_TYPENAME, GL_FALSE, bytes_per_face_vertex, (void*) 0);
	glDrawArrays(GL_TRIANGLES, 0, total_num_vertices);
	glDisableVertexAttribArray(0);

	disable_rendering_to_shadow_map(screen_size);
}

#endif
