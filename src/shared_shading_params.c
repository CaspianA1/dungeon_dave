#include "shared_shading_params.h"
#include "utils/macro_utils.h" // For `ARRAY_LENGTH`
#include "utils/opengl_wrappers.h" // For `use_shader`

static void init_constant_shading_params(UniformBuffer* const shading_params,
	const LevelRenderingConfig* const level_rendering_config,
	const vec2 all_bilinear_percents[num_unique_object_types],
	const CascadedShadowContext* const shadow_context) {

	enable_uniform_buffer_writing_batch(shading_params, true);

	// `UBO` = uniform buffer object
	#define UBO_WRITE(name) write_primitive_to_uniform_buffer(shading_params,\
		#name, &level_rendering_config -> name, sizeof(level_rendering_config -> name))

	write_array_of_primitives_to_uniform_buffer(shading_params,
		"all_bilinear_percents", (List) {
			.data = (void*) all_bilinear_percents,
			.item_size = sizeof(vec2),
			.length = num_unique_object_types
		}
	);

	UBO_WRITE(parallax_mapping.enabled); // TODO: write a 32-bit number instead? (Same for `tricubic_filtering_enabled`?)
	UBO_WRITE(parallax_mapping.min_layers); UBO_WRITE(parallax_mapping.max_layers);
	UBO_WRITE(parallax_mapping.height_scale); UBO_WRITE(parallax_mapping.lod_cutoff);

	UBO_WRITE(shadow_mapping.sample_radius);
	UBO_WRITE(shadow_mapping.esm_exponent);
	UBO_WRITE(shadow_mapping.esm_exponent_layer_scale_factor);

	UBO_WRITE(volumetric_lighting.num_samples);
	UBO_WRITE(volumetric_lighting.sample_density);
	UBO_WRITE(volumetric_lighting.opacity);

	write_array_of_primitives_to_uniform_buffer(shading_params,
		"shadow_mapping.cascade_split_distances", (List) {
			.data = shadow_context -> split_dists,
			.item_size = sizeof(GLfloat),
			/* TODO: make some macro somewhere to get the number of split dists,
			given the number of cascades (this logic is just hardcoded in at the moment) */
			.length = (buffer_size_t) shadow_context -> num_cascades - 1
		}
	);

	UBO_WRITE(ambient_occlusion.tricubic_filtering_enabled);
	UBO_WRITE(ambient_occlusion.strength);

	const sdl_pixel_component_t* const rgb_light_color = level_rendering_config -> rgb_light_color;
	const vec3 light_color = {rgb_light_color[0] / 255.0, rgb_light_color[1] / 255.0, rgb_light_color[2] / 255.0};
	write_primitive_to_uniform_buffer(shading_params, "light_color", light_color, sizeof(vec3));

	UBO_WRITE(tone_mapping_max_white);
	UBO_WRITE(noise_granularity);

	#undef UBO_WRITE

	disable_uniform_buffer_writing_batch(shading_params);
}

SharedShadingParams init_shared_shading_params(const GLuint* const shaders_that_share_params,
	const GLuint num_shaders, const LevelRenderingConfig* const level_rendering_config,
	const vec2 all_bilinear_percents[num_unique_object_types],
	const CascadedShadowContext* const shadow_context) {
	
	static const GLchar
		*const constant_subvar_names[] = {
			"all_bilinear_percents",

			"parallax_mapping.enabled",
			"parallax_mapping.min_layers", "parallax_mapping.max_layers",
			"parallax_mapping.height_scale", "parallax_mapping.lod_cutoff",

			"shadow_mapping.sample_radius", "shadow_mapping.esm_exponent",
			"shadow_mapping.esm_exponent_layer_scale_factor",
			"shadow_mapping.cascade_split_distances",

			"volumetric_lighting.num_samples",
			"volumetric_lighting.sample_density",
			"volumetric_lighting.opacity",

			"ambient_occlusion.tricubic_filtering_enabled",
			"ambient_occlusion.strength",

			"light_color", "tone_mapping_max_white", "noise_granularity"
		},

		*const dynamic_subvar_names[] = {
			"dir_to_light", "camera_pos_world_space",
			"billboard_front_facing_tbn",
			"view_projection", "view", "light_view_projection_matrices"
		};

	//////////

	const GLuint first_shader = shaders_that_share_params[0];

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

	init_constant_shading_params(&shared_shading_params.constant,
		level_rendering_config, all_bilinear_percents, shadow_context);

	for (GLuint i = 0; i < num_shaders; i++) {
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
	const Camera* const camera, const CascadedShadowContext* const shadow_context,
	const vec3 dir_to_light) {

	UniformBuffer* const dynamic_params = &shared_shading_params -> dynamic;

	const GLfloat* const right_xz = camera -> right_xz;
	const GLfloat right_xz_x = right_xz[0], right_xz_z = right_xz[1];

	//////////

	enable_uniform_buffer_writing_batch(dynamic_params, true);

	write_primitive_to_uniform_buffer(dynamic_params, "dir_to_light", dir_to_light, sizeof(vec3));
	write_primitive_to_uniform_buffer(dynamic_params, "camera_pos_world_space", camera -> pos, sizeof(vec3));

	write_matrix_to_uniform_buffer(dynamic_params, "billboard_front_facing_tbn", (GLfloat*) (mat3) {
		{-right_xz_x, 0.0f, -right_xz_z}, {0.0f, 1.0f, 0.0f}, {-right_xz_z, 0.0f, right_xz_x}
	}, sizeof(vec3), 3);

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
	const byte num_world_shaded_objects,
	const CascadedShadowContext* const shadow_context,
	const AmbientOcclusionMap* const ao_map,
	const GLuint materials_texture) {

	const GLuint
		ao_map_texture = ao_map -> texture,
		shadow_depth_layers = shadow_context -> depth_layers,
		shadow_depth_sampler = shadow_context -> plain_depth_sampler,
		shadow_depth_comparison_sampler = shadow_context -> depth_comparison_sampler;

	for (byte i = 0; i < num_world_shaded_objects; i++) {
		const WorldShadedObject wso = world_shaded_objects[i];
		const GLuint shader = wso.drawable -> shader;

		use_shader(shader);

		use_texture_in_shader(materials_texture, shader, "materials_sampler", TexPlain1D, TU_Materials);
		use_texture_in_shader(wso.drawable -> albedo_texture, shader, "albedo_sampler", TexSet, wso.texture_units.albedo);
		use_texture_in_shader(wso.drawable -> normal_map, shader, "normal_map_sampler", TexSet, wso.texture_units.normal_map);
		use_texture_in_shader(ao_map_texture, shader, "ambient_occlusion_sampler", TexVolumetric, TU_AmbientOcclusionMap);

		use_texture_in_shader(shadow_depth_layers, shader, "shadow_cascade_sampler", shadow_map_texture_type, TU_CascadedShadowMapPlain);
		glBindSampler(TU_CascadedShadowMapPlain, shadow_depth_sampler);

		use_texture_in_shader(shadow_depth_layers, shader, "shadow_cascade_sampler_depth_comparison", shadow_map_texture_type, TU_CascadedShadowMapDepthComparison);
		glBindSampler(TU_CascadedShadowMapDepthComparison, shadow_depth_comparison_sampler);
	}
}
