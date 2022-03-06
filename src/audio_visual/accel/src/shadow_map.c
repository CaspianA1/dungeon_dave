#ifndef SHADOW_MAP_C
#define SHADOW_MAP_C

/*
- Shadows for billboards and the weapon (they should read from the shadow map, not affect it)
- Limit orthographic matrix size for shadow map to map size
- Gaussian blur (that will make the shadows smooth)
- Can perhaps store penumbra size in 3rd component of moment texture

View frustum calculation:
- Need to capture whole scene
- Want an angle to the light as well

- So, can calculate position from those constraints
- Meaning that eventually, no position passed in
- So, given two angles, create a big plane
- Check that that plane is tight with the world's edge
- Then, make a cuboid extending from that plane to capture the whole world
*/

#include "headers/shadow_map.h"
#include "headers/constants.h"
#include "texture.c"
#include "camera.c"

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
	const GLsizei shadow_map_height, const vec3 light_pos,
	const GLfloat hori_angle, const GLfloat vert_angle) {

	////////// Generating a texture to hold the two moments

	const GLuint moment_texture = preinit_texture(TexPlain, TexNonRepeating);

	/*
	glTexParameteri(TexPlain, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(TexPlain, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(TexPlain, GL_TEXTURE_BORDER_COLOR, (GLfloat[4]) {1.0f, 1.0f, 1.0f, 1.0f});
	*/

	glTexImage2D(TexPlain, 0, GL_RG, shadow_map_width, shadow_map_height, 0, GL_RG, GL_FLOAT, NULL);

	////////// Generating a render buffer to act as a z-buffer

	GLuint depth_render_buffer;
	glGenRenderbuffers(1, &depth_render_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_render_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, shadow_map_width, shadow_map_height);

	////////// Defining a framebuffer, and attaching moment texture + depth render buffer to it

	GLuint framebuffer;
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, moment_texture, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_render_buffer);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		fail("make a framebuffer; framebuffer not complete", CreateFramebuffer);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//////////

	const GLuint shader = init_shader_program(depth_vertex_shader, depth_fragment_shader);

	vec3 light_dir;
	get_dir_in_2D_and_3D(hori_angle, vert_angle, (vec2) {0.0f, 0.0f}, light_dir);

	return (ShadowMapContext) {
		.shader_context = {shader, .INIT_UNIFORM(light_model_view_projection, shader)},

		.buffer_context = {
			framebuffer, moment_texture, depth_render_buffer,
			{shadow_map_width, shadow_map_height}
		},

		.light_context = {
			{light_pos[0], light_pos[1], light_pos[2]},
			{light_dir[0], light_dir[1], light_dir[2]},
			{0}
		}
	};
}

void deinit_shadow_map_context(const ShadowMapContext* const shadow_map_context) {
	deinit_texture(shadow_map_context -> buffer_context.moment_texture);
	glDeleteRenderbuffers(1, &shadow_map_context -> buffer_context.depth_render_buffer);
	glDeleteFramebuffers(1, &shadow_map_context -> buffer_context.framebuffer);
	glDeleteProgram(shadow_map_context -> shader_context.depth_shader);
}

static void get_model_view_projection_matrix_for_shadow_map(
	const ShadowMapContext* const shadow_map_context,
	const byte map_size[2], mat4 model_view_projection) {

	//////////

	/*
	Or, think like this:
	- Light must always cover whole scene
	- Only direction should vary
	- So, given a direction, find a position that encompasses the whole scene
	- This, in a way, reduces to the view frustum related problem from the internet

	- Simplified version (?)
	- Direction -> OBB that covers the whole scene
	- Only consider position that is out of scene to begin with
	- Perhaps consider a point on a sphere, looking at the map center
	- Adjusting where the point is on the sphere then changes the direction

	https://stackoverflow.com/questions/969798/plotting-a-point-on-the-edge-of-a-sphere
	*/

	(void) map_size;

	mat4 view, projection;
	const GLfloat far_clip_dist = 70.0f, l = sqrtf(50.0f * 50.0f + 50.0f * 50.0f + 18.0f * 18.0f); // 25.0f;
	glm_look_anyup((GLfloat*) shadow_map_context -> light_context.pos, (GLfloat*) shadow_map_context -> light_context.dir, view);
	glm_ortho(-l, l, l, -l, constants.camera.clip_dists.near, far_clip_dist, projection);
	glm_mul(projection, view, model_view_projection);
}

static void enable_rendering_to_shadow_map(ShadowMapContext* const shadow_map_context_ref, const byte map_size[2]) {
	ShadowMapContext shadow_map_context = *shadow_map_context_ref;

	////////// These matrices are relative to the light

	get_model_view_projection_matrix_for_shadow_map(shadow_map_context_ref,
		map_size, shadow_map_context.light_context.model_view_projection);

	////////// Activate shader, update light mvp, bind framebuffer, resize viewport, clear buffers, and cull front faces

	glUseProgram(shadow_map_context.shader_context.depth_shader);

	UPDATE_UNIFORM(shadow_map_context.shader_context.light_model_view_projection,
		Matrix4fv, 1, GL_FALSE, (GLfloat*) shadow_map_context.light_context.model_view_projection);

	glBindFramebuffer(GL_FRAMEBUFFER, shadow_map_context.buffer_context.framebuffer);
	glViewport(0, 0, shadow_map_context.buffer_context.size[0], shadow_map_context.buffer_context.size[1]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glCullFace(GL_FRONT);

	memcpy(shadow_map_context_ref -> light_context.model_view_projection,
		shadow_map_context.light_context.model_view_projection, sizeof(mat4));
}

static void disable_rendering_to_shadow_map(const int screen_size[2], const GLuint moment_texture) {
	// This generates mipmaps for the moment texture after it's been rendered
	glBindTexture(TexPlain, moment_texture);
	glGenerateMipmap(TexPlain);

	// Unbinding the framebuffer, resetting the viewport size, and turning on backface culling again
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, screen_size[0], screen_size[1]);
	glCullFace(GL_BACK);
}

void render_all_sectors_to_shadow_map(
	ShadowMapContext* const shadow_map_context,
	const BatchDrawContext* const sector_draw_context,
	const int screen_size[2], const byte map_size[2]) {

	glBindBuffer(GL_ARRAY_BUFFER, sector_draw_context -> buffers.gpu);

	GLint bytes_for_vertices;
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bytes_for_vertices);
	const GLsizei total_num_vertices = bytes_for_vertices / bytes_per_face * vertices_per_face;

	glBufferSubData(GL_ARRAY_BUFFER, 0, bytes_for_vertices, sector_draw_context -> buffers.cpu.data);

	enable_rendering_to_shadow_map(shadow_map_context, map_size);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, MESH_COMPONENT_TYPENAME, GL_FALSE, bytes_per_face_vertex, (void*) 0);
	glDrawArrays(GL_TRIANGLES, 0, total_num_vertices);
	glDisableVertexAttribArray(0);

	disable_rendering_to_shadow_map(screen_size, shadow_map_context -> buffer_context.moment_texture);
}

#endif
