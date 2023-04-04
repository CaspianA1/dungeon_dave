#ifndef SHARED_SHADING_PARAMS_H
#define SHARED_SHADING_PARAMS_H

#include "utils/uniform_buffer.h"
#include "glad/glad.h" // For OpenGL defs
#include "utils/typedefs.h" // For various typedefs
#include "level_config.h" // For `LevelRenderingConfig`
#include "data/constants.h" // For `num_unique_object_types`
#include "rendering/shadow.h" // For `CascadedShadowContext`
#include "camera.h" // For `Camera`
#include "cglm/cglm.h" // For `vec2`, and `vec3`
#include "rendering/drawable.h" // For `Drawable`
#include "utils/texture.h" // For `TextureUnit`
#include "rendering/ambient_occlusion.h" // For `AmbientOcclusionMap`

// I would like to call `constant` here `static`, but that's a keyword
typedef struct {UniformBuffer constant, dynamic;} SharedShadingParams;

// Excluded: init_constant_shading_params
SharedShadingParams init_shared_shading_params(const GLuint* const shaders_that_share_params,
	const GLuint num_shaders, const LevelRenderingConfig* const level_rendering_config,
	const vec2 all_bilinear_percents[num_unique_object_types],
	const CascadedShadowContext* const shadow_context);

void deinit_shared_shading_params(const SharedShadingParams* const shared_shading_params);

void update_shared_shading_params(SharedShadingParams* const shared_shading_params,
	const Camera* const camera, const CascadedShadowContext* const shadow_context,
	const vec3 dir_to_light);

//////////

/* World-shaded objects use the `world_shading.vert` and
`.frag` shaders, and appear in the 3D game world. */
typedef struct {
	const Drawable* const drawable;
	const struct {const TextureUnit albedo, normal_map;} texture_units;
} WorldShadedObject;

void init_shared_textures_for_world_shaded_objects(
	const WorldShadedObject* const world_shaded_objects,
	const byte num_world_shaded_objects,
	const CascadedShadowContext* const shadow_context,
	const AmbientOcclusionMap* const ao_map,
	const GLuint materials_texture);

#endif
