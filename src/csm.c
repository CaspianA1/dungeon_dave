#ifndef CSM_C // CSM = cascaded shadow mapping
#define CSM_C

// This is going to start off as plain old shadow mapping

#include "headers/csm.h"
#include "headers/camera.h"
#include "headers/texture.h"
#include "headers/constants.h"
#include "headers/shaders.h"
#include "headers/batch_draw_context.h"

#define DEPTH_TEXTURE_FORMAT GL_DEPTH_COMPONENT
#define INTERNAL_DEPTH_TEXTURE_FORMAT GL_DEPTH_COMPONENT16
#define DEPTH_TEXTURE_COMPONENT_TYPE GL_FLOAT

//////////

ShadowMapContext init_shadow_map_context(const GLsizei width,
	const GLsizei height, const vec3 light_pos, const GLfloat hori_angle,
	const GLfloat vert_angle, const GLfloat far_clip_dist) {

	ShadowMapContext s = {
		.light.far_clip_dist = far_clip_dist,
		.buffers = {
			.depth_texture = preinit_texture(TexPlain, TexNonRepeating,
				OPENGL_SHADOW_MAP_MAG_FILTER, OPENGL_SHADOW_MAP_MIN_FILTER),
			.size = {width, height}
		},

		.depth_shader = init_shader(depth_vertex_shader, depth_fragment_shader)
	};

	glm_vec3_copy((GLfloat*) light_pos, s.light.pos);
	get_dir_in_2D_and_3D(hori_angle, vert_angle, (vec2) {0.0f, 0.0f}, s.light.dir);

	glTexImage2D(TexPlain, 0, INTERNAL_DEPTH_TEXTURE_FORMAT, width,
		height, 0, DEPTH_TEXTURE_FORMAT, DEPTH_TEXTURE_COMPONENT_TYPE, NULL);

	glGenerateMipmap(TexPlain);

	//////////

	glGenFramebuffers(1, &s.buffers.frame);
	glBindFramebuffer(GL_FRAMEBUFFER, s.buffers.frame);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, s.buffers.depth_texture, 0);
	glDrawBuffer(GL_NONE);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		fail("make a framebuffer; framebuffer not complete", CreateFramebuffer);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//////////

	return s;
}

void deinit_shadow_map_context(const ShadowMapContext* const s) {
	deinit_shader(s -> depth_shader);
	deinit_texture(s -> buffers.depth_texture);
	glDeleteFramebuffers(1, &s -> buffers.frame);
}

void render_sectors_to_shadow_map(ShadowMapContext* const shadow_map_context,
	const BatchDrawContext* const sector_draw_context, const int screen_size[2]) {

	const GLuint depth_shader = shadow_map_context -> depth_shader;
	use_shader(depth_shader);

	ON_FIRST_CALL(
		mat4 view, projection;
		GLfloat* const model_view_projection = &shadow_map_context -> light.model_view_projection[0][0];
		const GLfloat fcd = shadow_map_context -> light.far_clip_dist;

		glm_look_anyup(shadow_map_context -> light.pos, shadow_map_context -> light.dir, view);
		glm_ortho(-fcd, fcd, fcd, -fcd, constants.camera.near_clip_dist, fcd, projection);
		glm_mul(projection, view, shadow_map_context -> light.model_view_projection);

		INIT_UNIFORM_VALUE(light_model_view_projection, depth_shader, Matrix4fv, 1, GL_FALSE, model_view_projection);
	);

	//////////

	glBindBuffer(GL_ARRAY_BUFFER, sector_draw_context -> buffers.gpu);

	GLint bytes_for_vertices;
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bytes_for_vertices);
	glBufferSubData(GL_ARRAY_BUFFER, 0, bytes_for_vertices, sector_draw_context -> buffers.cpu.data);

	WITH_VERTEX_ATTRIBUTE(false, 0, 3, FACE_MESH_COMPONENT_TYPENAME, bytes_per_face_vertex, 0,
		const ShadowMapBuffers buffers = shadow_map_context -> buffers;
		const GLsizei num_vertices = bytes_for_vertices / bytes_per_face * vertices_per_face;

		glViewport(0, 0, buffers.size[0], buffers.size[1]);
		glBindFramebuffer(GL_FRAMEBUFFER, buffers.frame);
		glCullFace(GL_FRONT);

		glClear(GL_DEPTH_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLES, 0, num_vertices);

		if (keys[KEY_MAKE_SHADOW_MAP_MIPMAP]) {
			glBindTexture(TexPlain, shadow_map_context -> buffers.depth_texture);
			glGenerateMipmap(TexPlain);
			glBindTexture(TexPlain, 0);
		};

		glCullFace(GL_BACK);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, screen_size[0], screen_size[1]);
	);
}

#endif
