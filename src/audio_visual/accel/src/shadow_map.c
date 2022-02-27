#ifndef SHADOW_MAP_C
#define SHADOW_MAP_C

/*
- Some odd results at very steep angles (aniso should fix that)
- Some tops of objects are shadowed when they shouldn't be
- Being close to an object blends the shadowed part with the unshadowed parj
- A light source for the shadow map context that isn't based on the camera
- Shadows for billboards and the weapon (they should read from the shadow map, not affect it)
- Limit orthographic matrix size for shadow map to map size (maybe not, if it doesn't affect anything)
- Gaussian blur (that will make the shadows smooth)
- Get mipmaps + aniso working too
- Can perhaps store penumbra size in 3rd component of moment texture
*/

#include "headers/shadow_map.h"
#include "headers/constants.h"
#include "headers/texture.h"

// TODO: to shaders.c
const GLchar *const depth_vertex_shader =
	"#version 330 core\n"

	"layout(location = 0) in vec3 vertex_pos_world_space;\n"

	"uniform mat4 light_model_view_projection;\n"

	"void main(void) {\n"
		"gl_Position = light_model_view_projection * vec4(vertex_pos_world_space, 1.0f);\n"
	"}\n",

*const depth_fragment_shader =
	"#version 330 core\n"

	"out vec2 moments;\n"

	"void main(void) {\n"
		"moments.x = gl_FragCoord.z;\n"
		"vec2 partial_derivatives = vec2(dFdx(moments.x), dFdy(moments.x));\n"
		"moments.y = moments.x * moments.x + 0.25f * dot(partial_derivatives, partial_derivatives);\n"
	"}\n";

ShadowMapContext init_shadow_map_context(const GLsizei shadow_map_width,
	const GLsizei shadow_map_height, const vec3 light_pos, const vec3 light_dir) {

	// Texture is for storing moments, and render buffer serves as a depth buffer
	GLuint framebuffer, moment_texture, depth_render_buffer;

	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	//////////

	glGenTextures(1, &moment_texture);
	glBindTexture(TexPlain, moment_texture);

	glTexParameteri(TexPlain, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(TexPlain, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexParameteri(TexPlain, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(TexPlain, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(TexPlain, GL_TEXTURE_BORDER_COLOR, (GLfloat[4]) {1.0f, 1.0f, 1.0f, 1.0f});  

	glTexImage2D(TexPlain, 0, GL_RG, shadow_map_width, shadow_map_height, 0, GL_RG, GL_FLOAT, NULL);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, moment_texture, 0);

	//////////

	glGenRenderbuffers(1, &depth_render_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_render_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, shadow_map_width, shadow_map_height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_render_buffer);

	//////////

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		fail("make a framebuffer; framebuffer not complete", CreateFramebuffer);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//////////

	const GLuint shader = init_shader_program(depth_vertex_shader, depth_fragment_shader);

	return (ShadowMapContext) {
		.shader_context = {shader, .INIT_UNIFORM(light_model_view_projection, shader)},

		.buffer_context = {
			framebuffer, moment_texture, depth_render_buffer,
			{shadow_map_width, shadow_map_height}
		},

		.light_context = {
			{light_pos[0], light_pos[1], light_pos[2]},
			{light_dir[0], light_dir[1], light_dir[2]},
			.model_view_projection = {0}
		}
	};
}

void deinit_shadow_map_context(const ShadowMapContext* const shadow_map_context) {
	deinit_texture(shadow_map_context -> buffer_context.moment_texture);
	glDeleteRenderbuffers(1, &shadow_map_context -> buffer_context.depth_render_buffer);
	glDeleteFramebuffers(1, &shadow_map_context -> buffer_context.framebuffer);
	glDeleteProgram(shadow_map_context -> shader_context.depth_shader);
}

static void enable_rendering_to_shadow_map(ShadowMapContext* const shadow_map_context_ref) {
	ShadowMapContext shadow_map_context = *shadow_map_context_ref;

	////////// These matrices are relative to the light

	mat4 view, projection;

	glm_look_anyup(shadow_map_context.light_context.pos, shadow_map_context.light_context.dir, view);
	glm_ortho(-50.0f, 50.0f, -50.0f, 50.0f, constants.camera.clip_dists.near, constants.camera.clip_dists.far, projection);

	glm_mul(projection, view, shadow_map_context.light_context.model_view_projection);

	////////// Activate shader, update light mvp, bind framebuffer, resize viewport, clear framebuffer, cull front faces

	glUseProgram(shadow_map_context.shader_context.depth_shader);

	UPDATE_UNIFORM(shadow_map_context.shader_context.light_model_view_projection,
		Matrix4fv, 1, GL_FALSE, (GLfloat*) shadow_map_context.light_context.model_view_projection);

	glBindFramebuffer(GL_FRAMEBUFFER, shadow_map_context.buffer_context.framebuffer);
	glViewport(0, 0, shadow_map_context.buffer_context.size[0], shadow_map_context.buffer_context.size[1]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear depth map
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
