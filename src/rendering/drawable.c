#include "rendering/drawable.h"
#include "utils/opengl_wrappers.h" // For various OpenGL wrappers

////////// The main Drawable functions

Drawable init_drawable_with_vertices(
	void (*const vertex_spec_definer) (void), const uniform_updater_t uniform_updater,
	const GLenum vertex_buffer_access, const GLenum triangle_mode,
	const List vertices, const GLuint shader, const GLuint albedo_texture,
	const GLuint normal_map) {

	const GLuint vertex_buffer = init_gpu_buffer(), vertex_spec = init_vertex_spec();

	use_vertex_buffer(vertex_buffer);
	init_vertex_buffer_data(vertices.length, vertices.item_size, vertices.data, vertex_buffer_access);

	use_vertex_spec(vertex_spec);
	vertex_spec_definer();

	return (Drawable) {
		triangle_mode, vertex_spec, vertex_buffer,
		shader, albedo_texture, normal_map, uniform_updater
	};
}

Drawable init_drawable_without_vertices(const uniform_updater_t uniform_updater,
	const GLenum triangle_mode, const GLuint shader, const GLuint albedo_texture,
	const GLuint normal_map) {

	return (Drawable) {triangle_mode, 0, 0, shader, albedo_texture, normal_map, uniform_updater};
}

void deinit_drawable(const Drawable drawable) {
	deinit_shader(drawable.shader);

	if (drawable.albedo_texture != 0) deinit_texture(drawable.albedo_texture);
	if (drawable.normal_map != 0) deinit_texture(drawable.normal_map);
	if (drawable.vertex_buffer != 0) deinit_gpu_buffer(drawable.vertex_buffer);
	if (drawable.vertex_spec != 0) deinit_vertex_spec(drawable.vertex_spec);
}

void draw_drawable(const Drawable drawable,
	const buffer_size_t num_vertices, const buffer_size_t num_instances,
	const void* const uniform_updater_param, const byte invocation_params) {

	if (invocation_params & UseShaderPipeline) {
		use_shader(drawable.shader);

		if (drawable.uniform_updater != NULL)
			drawable.uniform_updater(uniform_updater_param);
	}

	if (invocation_params & UseVertexSpec) use_vertex_spec(drawable.vertex_spec);

	if (num_instances == 0) draw_primitives(drawable.triangle_mode, (GLsizei) num_vertices);
	else draw_instances(drawable.triangle_mode, (GLsizei) num_vertices, (GLsizei) num_instances);
}
