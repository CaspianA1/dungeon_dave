#ifndef LEVEL_CONFIG_H
#define LEVEL_CONFIG_H

#include "utils/typedefs.h" // For OpenGL types + other typedefs
#include "utils/list.h" // For `List`
#include "animation.h" // For `AnimationLayout`
#include "utils/normal_map_generation.h" // For `NormalMapConfig`
#include <stdbool.h> // For `bool`
#include "rendering/shadow.h" // For `CascadedShadowContextConfig`
#include "rendering/dynamic_light.h" // For `DynamicLightConfig`
#include "rendering/entities/skybox.h" // For `SkyboxConfig`
#include "utils/texture.h" // For `sdl_pixel_component_t`

//////////

typedef uint32_t packed_material_properties_t;

typedef struct {
	const GLchar* const albedo_texture_path; // TODO: put the heightmap scale in the properties
	const packed_material_properties_t properties; // Metallicity, min roughness, and max roughness
} MaterialPropertiesPerObjectInstance;

typedef struct {
	const GLuint material_properties_buffer, buffer_texture;
} MaterialsTexture;

// Excluded: copy_matching_material_to_dest_materials

MaterialsTexture init_materials_texture(const List* const all_materials, const List* const sector_face_texture_paths,
	const List* const still_billboard_texture_paths, const List* const billboard_animation_layouts,
	List* const billboards, const AnimationLayout* const weapon_sprite_animation_layout,
	material_index_t* const weapon_sprite_material_index);

void deinit_materials_texture(const MaterialsTexture* const materials_texture);

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
		const byte sample_radius, esm_exponent;

		const GLfloat esm_exponent_layer_scale_factor, billboard_alpha_threshold;
		const CascadedShadowContextConfig cascaded_shadow_config;
	} shadow_mapping;

	// Note: if the opacity is 0, volumetric lighting is not computed
	const struct {
		const byte num_samples;
		const GLfloat sample_density, opacity;
	} volumetric_lighting;

	const struct {
		const bool tricubic_filtering_enabled;
		const GLfloat strength;
	} ambient_occlusion;

	const DynamicLightConfig dynamic_light_config;
	const SkyboxConfig skybox_config;

	const sdl_pixel_component_t rgb_light_color[3];
	const GLfloat ambient_strength, tone_mapping_max_white, noise_granularity;
} LevelRenderingConfig;

#endif
