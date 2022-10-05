#include "shared_shading_params.h"
#include "utils/opengl_wrappers.h"

static void init_constant_shading_params(UniformBuffer* const shading_params, const CascadedShadowContext* const shadow_context) {
	enable_uniform_buffer_writing_batch(shading_params, true);

	#define UBO_WRITE(name) write_primitive_to_uniform_buffer(shading_params, #name, &constants.lighting.name, sizeof(constants.lighting.name))

	UBO_WRITE(percents.bilinear); UBO_WRITE(percents.ao);

	UBO_WRITE(strengths.ambient); UBO_WRITE(strengths.diffuse);
	UBO_WRITE(strengths.specular); UBO_WRITE(specular_exponents.matte);
	UBO_WRITE(specular_exponents.rough);

	write_array_of_primitives_to_uniform_buffer(shading_params, "cascade_split_distances", (List) {
		.data = shadow_context -> split_dists,
		.item_size = sizeof(GLfloat),
		.length = (buffer_size_t) shadow_context -> num_cascades - 1
	});

	UBO_WRITE(tone_mapping_max_white);
	UBO_WRITE(noise_granularity);
	UBO_WRITE(overall_scene_tone);

	#undef UBO_WRITE

	disable_uniform_buffer_writing_batch(shading_params);
}

SharedShadingParams init_shared_shading_params(const GLuint* const shaders_that_share_params,
	const buffer_size_t num_shaders, const CascadedShadowContext* const shadow_context) {
	
	const GLuint first_shader = shaders_that_share_params[0];

	const GLchar* const constant_subvar_names[] = {
		"percents.bilinear", "percents.ao",
		"strengths.ambient", "strengths.diffuse", "strengths.specular",

		"specular_exponents.matte", "specular_exponents.rough",

		"cascade_split_distances",

		"tone_mapping_max_white", "noise_granularity", "overall_scene_tone"
	};

	static const GLchar* const dynamic_subvar_names[] = {
		"dir_to_light", "camera_pos_world_space",
		"view_projection", "view", "light_view_projection_matrices"
	};

	SharedShadingParams shared_shading_params = {
		.constant = init_uniform_buffer(
			GL_STATIC_DRAW, "ConstantShadingParams",
			first_shader, constant_subvar_names, ARRAY_LENGTH(constant_subvar_names)
		),
		.dynamic = init_uniform_buffer(
			GL_DYNAMIC_DRAW, "DynamicShadingParams",
			first_shader, dynamic_subvar_names, ARRAY_LENGTH(dynamic_subvar_names)
		)
	};

	init_constant_shading_params(&shared_shading_params.constant, shadow_context);

	for (buffer_size_t i = 0; i < num_shaders; i++) {
		const GLuint shader = shaders_that_share_params[i];
		bind_uniform_buffer_to_shader(&shared_shading_params.constant, shader);
		bind_uniform_buffer_to_shader(&shared_shading_params.dynamic, shader);
	}

	return shared_shading_params;
}

void deinit_shared_shading_params(const SharedShadingParams* const shared_shading_params) {
	deinit_uniform_buffer(&shared_shading_params -> constant);
	deinit_uniform_buffer(&shared_shading_params -> dynamic);
}

void update_shared_shading_params(SharedShadingParams* const shared_shading_params,
	const Camera* const camera, const CascadedShadowContext* const shadow_context) {

	UniformBuffer* const dynamic_params = &shared_shading_params -> dynamic;

	enable_uniform_buffer_writing_batch(dynamic_params, true);

	write_primitive_to_uniform_buffer(dynamic_params, "dir_to_light", shadow_context -> dir_to_light, sizeof(vec3));
	write_primitive_to_uniform_buffer(dynamic_params, "camera_pos_world_space", camera -> pos, sizeof(vec3));
	write_matrix_to_uniform_buffer(dynamic_params, "view_projection", (GLfloat*) camera -> view_projection, sizeof(vec4), 4);
	write_matrix_to_uniform_buffer(dynamic_params, "view", (GLfloat*) camera -> view, sizeof(vec4), 4);

	write_array_of_matrices_to_uniform_buffer(dynamic_params, "light_view_projection_matrices",
		(const GLfloat**) shadow_context -> light_view_projection_matrices,
		(buffer_size_t) shadow_context -> num_cascades, sizeof(vec4), 4
	);

	disable_uniform_buffer_writing_batch(dynamic_params);
}

//////////

void init_shared_textures_for_world_shaded_objects(
	const WorldShadedObject* const world_shaded_objects,
	const byte num_world_shaded_objects, const Skybox* const skybox,
	const CascadedShadowContext* const shadow_context,
	const AmbientOcclusionMap* const ao_map) {

	const GLuint
		skybox_diffuse_texture = skybox -> diffuse_texture,
		shadow_depth_layers = shadow_context -> depth_layers,
		ao_map_texture = ao_map -> texture;

	for (byte i = 0; i < num_world_shaded_objects; i++) {
		const WorldShadedObject wso = world_shaded_objects[i];
		const GLuint shader = wso.drawable -> shader;

		use_shader(shader);

		use_texture_in_shader(skybox_diffuse_texture, shader, "environment_map_sampler", TexSkybox, TU_Skybox);
		use_texture_in_shader(wso.drawable -> diffuse_texture, shader, "diffuse_sampler", TexSet, wso.texture_units.diffuse);
		use_texture_in_shader(wso.drawable -> normal_map, shader, "normal_map_sampler", TexSet, wso.texture_units.normal_map);
		use_texture_in_shader(shadow_depth_layers, shader, "shadow_cascade_sampler", shadow_map_texture_type, TU_CascadedShadowMap);
		use_texture_in_shader(ao_map_texture, shader, "ambient_occlusion_sampler", TexVolumetric, TU_AmbientOcclusionMap);
	}
}
