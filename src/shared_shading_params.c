#include "shared_shading_params.h"

static void init_constant_shading_params(UniformBuffer* const shading_params, const CascadedShadowContext* const shadow_context) {
	enable_uniform_buffer_writing_batch(shading_params, true);

	#define UBO_WRITE(name) write_primitive_to_uniform_buffer(shading_params, #name, &constants.lighting.name, sizeof(constants.lighting.name))

	UBO_WRITE(strengths.ambient);
	UBO_WRITE(strengths.diffuse);
	UBO_WRITE(strengths.specular);
	UBO_WRITE(specular_exponent_domain);
	UBO_WRITE(tone_mapping.enabled);
	UBO_WRITE(tone_mapping.max_white);
	UBO_WRITE(noise_granularity);
	UBO_WRITE(overall_scene_tone);

	#undef UBO_WRITE

	write_primitive_to_uniform_buffer(shading_params, "dir_to_light", shadow_context -> dir_to_light, sizeof(vec3));

	write_array_of_primitives_to_uniform_buffer(shading_params, "cascade_split_distances", (List) {
		.data = shadow_context -> split_dists,
		.item_size = sizeof(GLfloat),
		.length = (buffer_size_t) shadow_context -> num_cascades - 1
	});

	disable_uniform_buffer_writing_batch(shading_params);
}

SharedShadingParams init_shared_shading_params(const GLuint* const shaders_that_share_params,
	const buffer_size_t shader_count, const CascadedShadowContext* const shadow_context) {
	
	const GLuint first_shader = shaders_that_share_params[0];

	const GLchar* const constant_subvar_names[] = {
		"strengths.ambient", "strengths.diffuse", "strengths.specular",
		"specular_exponent_domain", "tone_mapping.enabled", "tone_mapping.max_white",
		"noise_granularity", "overall_scene_tone", "dir_to_light", "cascade_split_distances"
	};

	static const GLchar* const dynamic_subvar_names[] = {
		"camera_pos_world_space", "view_projection",
		"view", "light_view_projection_matrices"
	};

	SharedShadingParams shared_shading_params = {
		.constant = init_uniform_buffer(
			GL_STATIC_DRAW, "ConstantShadingParams", 0,
			first_shader, constant_subvar_names, ARRAY_LENGTH(constant_subvar_names)
		),
		.dynamic = init_uniform_buffer(
			GL_DYNAMIC_DRAW, "DynamicShadingParams", 1,
			first_shader, dynamic_subvar_names, ARRAY_LENGTH(dynamic_subvar_names)
		)
	};

	init_constant_shading_params(&shared_shading_params.constant, shadow_context);

	for (buffer_size_t i = 0; i < shader_count; i++) {
		const GLuint shader = shaders_that_share_params[i];
		bind_uniform_buffer_to_shader(&shared_shading_params.constant, shader);
		bind_uniform_buffer_to_shader(&shared_shading_params.dynamic, shader);
	}

	return shared_shading_params;
}

void deinit_shared_shading_parsms(const SharedShadingParams* const shared_shading_params) {
	deinit_uniform_buffer(&shared_shading_params -> constant);
	deinit_uniform_buffer(&shared_shading_params -> dynamic);
}

void update_shared_shading_params(SharedShadingParams* const shared_shading_params,
	const Camera* const camera, const CascadedShadowContext* const shadow_context) {

	UniformBuffer* const dynamic_params = &shared_shading_params -> dynamic;

	enable_uniform_buffer_writing_batch(dynamic_params, true);

	write_primitive_to_uniform_buffer(dynamic_params, "camera_pos_world_space", camera -> pos, sizeof(vec3));
	write_matrix_to_uniform_buffer(dynamic_params, "view_projection", (GLfloat*) camera -> view_projection, sizeof(vec4), 4);
	write_matrix_to_uniform_buffer(dynamic_params, "view", (GLfloat*) camera -> view, sizeof(vec4), 4);

	write_array_of_matrices_to_uniform_buffer(dynamic_params, "light_view_projection_matrices",
		(const GLfloat**) shadow_context -> light_view_projection_matrices,
		(buffer_size_t) shadow_context -> num_cascades, sizeof(vec4), 4
	);

	disable_uniform_buffer_writing_batch(dynamic_params);
}
