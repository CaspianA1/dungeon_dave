#ifndef LEVEL_CONFIG_H
#define LEVEL_CONFIG_H

#include "utils/typedefs.h" // For OpenGL types + other typedefs
#include "utils/list.h" // For `List`
#include "animation.h" // For `AnimationLayout`
#include "utils/normal_map_generation.h" // For `NormalMapConfig`
#include <stdbool.h> // For `bool`
#include "rendering/shadow.h" // For `CascadedShadowContextConfig`
#include "rendering/dynamic_light.h" // For `DynamicLightConfig`

//////////

typedef struct {
	const GLchar* const albedo_texture_path; // TODO: put the heightmap scale in here
	const struct {const GLfloat metallicity, min_roughness, max_roughness;} lighting;
} MaterialPropertiesPerObjectInstance;

// Excluded: copy_matching_material_to_dest_materials

void validate_all_materials(const List* const all_materials);

GLuint init_materials_texture(const List* const all_materials, const List* const sector_face_texture_paths,
	const List* const still_billboard_texture_paths, const List* const billboard_animation_layouts,
	List* const billboards, const AnimationLayout* const weapon_sprite_animation_layout,
	material_index_t* const weapon_sprite_material_index);

//////////

typedef struct {
	const GLsizei texture_rescale_size;
	const struct {const GLfloat albedo, normal;} bilinear_percents;
	const NormalMapConfig normal_map_config; // TODO: eventually, put this in `MaterialPropertiesPerObjectInstance`
} MaterialPropertiesPerObjectType;

//////////

typedef struct {
	const struct {
		const bool enabled;
		const GLfloat min_layers, max_layers, height_scale, lod_cutoff;
	} parallax_mapping;

	const struct {
		const GLuint sample_radius, esm_exponent;
		const GLfloat esm_exponent_layer_scale_factor, billboard_alpha_threshold;
		const CascadedShadowContextConfig shadow_context_config;
	} shadow_mapping;

	const struct {
		const bool enabled;
		const GLuint num_samples;
		const GLfloat decay, decay_weight, sample_density, opacity;
	} volumetric_lighting;

	const struct {
		const bool tricubic_filtering_enabled;
		const GLfloat strength;
	} ambient_occlusion;

	const DynamicLightConfig dynamic_light_config;
	const GLchar* const skybox_path;

	const vec3 light_color;
	const GLfloat ambient_strength, tone_mapping_max_white, noise_granularity;
} LevelRenderingConfig;

#endif
